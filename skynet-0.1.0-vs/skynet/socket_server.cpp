#include "socket_server.h"
#include "socket_helper.h"
#include "socket_buffer_pool.h"

#include "socket_iocp.h"

#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define CMD_EXIT		0x00000000
#define CMD_ACCEPT		0xFFFFFFF1
#define CMD_DISCONNECT	0xFFFFFFF2

#define IOCP_GOON		0
#define IOCP_CONTINUE	1
#define IOCP_BREAK		2

#define MAX_INFO 128
// MAX_SOCKET will be 2^MAX_SOCKET_P
#define MAX_SOCKET_P 16
#define MAX_EVENT 64
#define MIN_READ_BUFFER 64
#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_RESERVE 1
#define SOCKET_TYPE_PLISTEN 2
#define SOCKET_TYPE_LISTEN 3
#define SOCKET_TYPE_CONNECTING 4
#define SOCKET_TYPE_CONNECTED 5
#define SOCKET_TYPE_HALFCLOSE 6
#define SOCKET_TYPE_PACCEPT 7
#define SOCKET_TYPE_BIND 8

#define MAX_SOCKET (1<<MAX_SOCKET_P)

struct socket_server
{
	LPFN_CONNECTEX				pfn_connectex;
	LPFN_ACCEPTEX				pfn_acceptex;
	LPFN_GETACCEPTEXSOCKADDRS	pfn_get_acceptex_sockaddrs;
	LPFN_DISCONNECTEX			pfn_disconnectex;
	iocp_fd						event_fd;					// iocp句柄
	int							thread;						// 工作者线程数
	HANDLE*						handles;					// 工作者线程句柄列表
	//struct socket				slot[MAX_SOCKET];			// 应用层socket池
	struct socket*				slot;						// 应用层socket池
	volatile long				alloc_id;					// 用于给每个应用层socket分配一个唯一id
	struct socket_buffer_pool*	pool;						// 空闲的缓冲区列表
	socket_result_callback		callback;					// socket消息回调
};

static int reserve_id(struct socket_server *ss);

static unsigned __stdcall worker_thread(void* p);
static int ctrl_cmd(struct socket_server *ss, OVERLAPPED* ol, DWORD dwBytes, struct socket* s);

static void handle_io(struct socket_server* ss, struct socket* s, struct socket_buffer* sb, DWORD dwBytes, DWORD err);
static void handle_connect(struct socket_server* ss, struct socket* s, struct socket_buffer* sb);
static void handle_accept(struct socket_server* ss, struct socket* listen_sock, struct socket_buffer* sb);
static void handle_send(struct socket_server* ss, struct socket* s, struct socket_buffer* sb);
static void handle_receive(struct socket_server* ss, struct socket* s, struct socket_buffer* sb);
static void handle_error(struct socket_server* ss, struct socket* s, struct socket_buffer* sb, DWORD err);

static void do_error(struct socket_server* ss, struct socket* s, struct socket_buffer* sb, int err);
static int do_listen(const char * host, int port, int backlog);
static int do_accept(struct socket_server *ss, struct socket* s);
static int do_send(struct socket_server *ss, struct socket* s, const char* buffer, int sz);
static int do_receive(struct socket_server* ss, struct socket* s, struct socket_buffer* sb);
static void do_close(struct socket *s);

// 申请一个应用层socket id
static int reserve_id(struct socket_server *ss) {
	for (int i=0;i<MAX_SOCKET;i++) {
		//int id = __sync_add_and_fetch(&(ss->alloc_id), 1);
		int id = InterlockedIncrement(&(ss->alloc_id));
		if (id < 0) {
			//id = __sync_and_and_fetch(&(ss->alloc_id), 0x7fffffff);
			InterlockedExchange(&(ss->alloc_id), 0);
			id = 0;
		}
		struct socket *s = &ss->slot[id % MAX_SOCKET];
		if (s->type == SOCKET_TYPE_INVALID) {
			/*if (__sync_bool_compare_and_swap(&s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)) {
				return id;*/
			uint32_t ret = InterlockedCompareExchange(&s->type, SOCKET_TYPE_RESERVE, SOCKET_TYPE_INVALID);
			if (ret != SOCKET_TYPE_RESERVE)
				return id;
			else {
				// retry
				--i;
			}
		}
	}
	return -1;
}

static unsigned __stdcall worker_thread(void* p)
{
	struct socket_server* ss = (struct socket_server*)p;
	while (TRUE)
	{
		DWORD err = NO_ERROR;

		DWORD bytes_transferred;
		OVERLAPPED* ol;
		struct socket* s;

		// TODO，GetQueuedCompletionStatus改为GetQueuedCompletionStatusEx
		BOOL result = GetQueuedCompletionStatus(
			ss->event_fd,
			&bytes_transferred,
			(PULONG_PTR)&s,
			&ol,
			INFINITE);


		if(ol == NULL)
		{
			int ret = ctrl_cmd(ss, ol, bytes_transferred, s);		// 处理控制命令

			if(ret == IOCP_CONTINUE)
				continue;
			else if(ret == IOCP_BREAK)
				break;
		}

		struct socket_buffer* sb	= CONTAINING_RECORD(ol, struct socket_buffer, overlapped);

		if (!result)
		{
			DWORD flag	= 0;
			err = GetLastError();

			result = WSAGetOverlappedResult(s->fd, &sb->overlapped, &bytes_transferred, FALSE, &flag);

			if (!result)
				err = WSAGetLastError();


		}

		handle_io(ss, s, sb, bytes_transferred, err);
	}

	return 0;
}

static int ctrl_cmd(struct socket_server *ss, OVERLAPPED* ol, DWORD cmd, struct socket* s)
{
	int result = 0;

	if(cmd == CMD_ACCEPT)
	{
		do_accept(ss, s);
		result = IOCP_CONTINUE;
	}
	else if(cmd == CMD_DISCONNECT)
	{
		do_close(s);
		result = IOCP_CONTINUE;
	}
	else if(cmd == CMD_EXIT && s == NULL)
		result = IOCP_BREAK;

	return result;
}

static void handle_io(struct socket_server* ss, struct socket* s, struct socket_buffer* sb, DWORD dwBytes, DWORD dwErrorCode)
{
	assert(sb != NULL);
	assert(s != NULL);

	if(dwErrorCode != NO_ERROR)
	{
		handle_error(ss, s, sb, dwErrorCode);
		return;
	}

	if(dwBytes == 0 && sb->operation != OP_ACCEPT && sb->operation != OP_CONNECT)
	{
		struct socket_message result;
		result.data = NULL;
		result.id = s->id;
		result.opaque = s->opaque;
		ss->callback(SOCKET_CLOSE, &result);			// 回调

		s->type = SOCKET_TYPE_INVALID;
		socket_buffer_pool_put(ss->pool, sb);
		return;
	}

	sb->buffer.len = dwBytes;

	switch(sb->operation)
	{
	case OP_CONNECT:
		handle_connect(ss, s, sb);
		break;
	case OP_ACCEPT:
		handle_accept(ss, s, sb);
		break;
	case OP_SEND:
		handle_send(ss, s, sb);
		break;
	case OP_RECEIVE:
		handle_receive(ss, s, sb);
		break;
	default:
		assert(FALSE);
	}
}

// 连接完成
static void handle_connect(struct socket_server* ss, struct socket* s, struct socket_buffer* sb)
{
	s->type = SOCKET_TYPE_CONNECTED;

	update_connect_context(s->fd);
	keep_alive(s->fd, TRUE);

	struct socket_message result;
	result.data = NULL;
	result.id = s->id;
	result.opaque = s->opaque;
	ss->callback(SOCKET_OPEN, &result);			// 回调

	do_receive(ss, s, sb);
}

// 接受连接完成
static void handle_accept(struct socket_server* ss, struct socket* s, struct socket_buffer* sb)
{
	PostQueuedCompletionStatus(ss->event_fd, CMD_ACCEPT, (ULONG_PTR)s, NULL);

	int addrlen;
	int remote_addrlen;
	struct sockaddr* addr;
	struct sockaddr* remote_addr;

	ss->pfn_get_acceptex_sockaddrs(
		sb->buffer.buf,
		0,
		sizeof(struct sockaddr_in) + 16,
		sizeof(struct sockaddr_in) + 16,
		(SOCKADDR **)&addr,
		&addrlen,
		(SOCKADDR **)&remote_addr,
		&remote_addrlen);

	int id = reserve_id(ss);
	struct socket* ns = &ss->slot[id % MAX_SOCKET];
	ns->fd = sb->sock;
	ns->id = id;
	ns->type = SOCKET_TYPE_CONNECTED;
	ns->opaque = s->opaque;

	update_accept_context(sb->sock, s->fd);
	keep_alive(sb->sock, TRUE);

	struct socket_message result;
	result.data = NULL;
	result.id = s->id;
	result.opaque = ns->opaque;
	result.ud = ns->id;

	char buffer[MAX_INFO];
	strcpy_s(buffer, inet_ntoa(((SOCKADDR_IN*)remote_addr)->sin_addr));
	result.data = buffer;

	ss->callback(SOCKET_ACCEPT, &result);			// 回调

	// 将新的已连接套接字与完成端口绑定
	CreateIoCompletionPort((HANDLE)sb->sock, ss->event_fd, (ULONG_PTR)ns, 0);
	do_receive(ss, ns, sb);		// 投递异步接收请求
}

// 发送完成
static void handle_send(struct socket_server* ss, struct socket* s, struct socket_buffer* sb)
{
	struct socket_message result;
	result.data = NULL;
	result.id = s->id;
	result.opaque = s->opaque;
	result.ud = sb->buffer.len;

	ss->callback(SOCKET_SENT, &result);			// 回调

	// 发送完毕，将缓冲区归还至缓冲池
	socket_buffer_pool_put(ss->pool, sb);
}

// 接收完成
static void handle_receive(struct socket_server* ss, struct socket* s, struct socket_buffer* sb)
{
	struct socket_message result;
	result.data = sb->buffer.buf;
	result.id = s->id;
	result.opaque = s->opaque;
	result.ud = sb->buffer.len;
	ss->callback(SOCKET_DATA, &result);			// 回调

	do_receive(ss, s, sb);						// 接收完毕，继续投递异步接收请求
}

static void handle_error(struct socket_server* ss, struct socket* s, struct socket_buffer* sb, DWORD err)
{
	// 出现ERROR_OPERATION_ABORTED，可能是调用了CancelIo或CancelIoEx或closesocket;
	if (err == ERROR_OPERATION_ABORTED)
	{
		socket_buffer_pool_put(ss->pool, sb);
		return;
	}

	if(sb->operation != OP_ACCEPT)
		do_error(ss, s, sb, err);
	else
	{
		do_close(s);
		//::PostQueuedCompletionStatus(ss->event_fd, CMD_ACCEPT, 0, NULL);
	}
	socket_buffer_pool_put(ss->pool, sb);
	
}

static void do_error(struct socket_server* ss, struct socket* s, struct socket_buffer* sb, int err)
{
	if(err != WSAENOTSOCK)
	{
		do_close(s);
	}

	struct socket_message result;
	result.data = NULL;
	result.id = s->id;
	result.opaque = s->opaque;

	char buffer[MAX_INFO] = {0};
	result.data = buffer;

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, buffer, MAX_INFO, NULL);

	s->type = SOCKET_TYPE_INVALID;		// 归还socket
	ss->callback(SOCKET_ERROR2, &result);			// 回调
}

static int do_listen(const char * host, int port, int backlog)
{
	// socket、bind、listen
	uint32_t addr = INADDR_ANY;
	if (host[0]) {
		addr=inet_addr(host);
	}
	int listen_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_fd < 0) {
		return -1;
	}
	int reuse = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int))==-1) {
		goto _failed;
	}

	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = addr;
	if (bind(listen_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		goto _failed;
	}
	if (listen(listen_fd, backlog) == -1) {
		goto _failed;
	}
	return listen_fd;
_failed:
	closesocket(listen_fd);
	return -1;
}

static int do_accept(struct socket_server *ss, struct socket* s)
{
	int ret;
	SOCKET sock	= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct socket_buffer*	sb	= socket_buffer_pool_get(ss->pool);

	assert(sock != INVALID_SOCKET);

	ret = post_accept(ss->pfn_acceptex, s->fd, sock, sb);
	if (ret != NO_ERROR)
	{
		close_socket(sock);
		socket_buffer_pool_put(ss->pool, sb);
	}

	return ret;
}

static int do_send(struct socket_server *ss, struct socket* s, const char* buffer, int sz)
{
	int result	= NO_ERROR;
	struct socket_buffer* sb;
	while (sz > 0)
	{
		int send_size = sz > DEFAULT_SOCKET_BUFFER_SIZE ? DEFAULT_SOCKET_BUFFER_SIZE : sz;
		sb = socket_buffer_pool_get(ss->pool);
		memcpy(sb->buffer.buf, buffer, send_size);
		sb->buffer.len = send_size;
		result = post_send(s->fd, sb);
		if (result != NO_ERROR)
		{
			socket_buffer_pool_put(ss->pool, sb);
			break;
		}
		sz -= send_size;
		buffer += send_size;
	}

	return result;
}

static int do_receive(struct socket_server* ss, struct socket* s, socket_buffer* sb)
{
	sb->buffer.len = DEFAULT_SOCKET_BUFFER_SIZE;
	int result = post_receive(s->fd, sb);

	if(result != NO_ERROR)
	{
		do_error(ss, s, sb, result);
		socket_buffer_pool_put(ss->pool, sb);
	}

	return result;
}

static void do_close(struct socket *s) {
	if (s->type == SOCKET_TYPE_INVALID) {
		return;
	}
	assert(s->type != SOCKET_TYPE_RESERVE);

	if (s->type != SOCKET_TYPE_BIND) {
		//shutdown(s->fd, SD_SEND);
		//closesocket(s->fd);
		close_socket(s->fd);
	}
	s->type = SOCKET_TYPE_INVALID;		// 归还socket
}

struct socket_server* socket_server_create(socket_result_callback callback, int thread)
{
	WSADATA wsaData;

	// 启动WinSock
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0)
	{
		fprintf(stderr, "socket-server: WSAStartup failed with error: %d.\n", err);
		return NULL;
	}

	// 创建完成端口
	iocp_fd fd = iocp_create();
	if (iocp_invalid(fd))
	{
		fprintf(stderr, "socket-server: create event pool failed.\n");
		return NULL;
	}
	
	// 创建socket_server
	struct socket_server* ss = (struct socket_server*)malloc(sizeof(*ss));
	ss->event_fd = fd;

	ss->slot = (struct socket*)malloc(sizeof(struct socket)*MAX_SOCKET);

	// 初始化应用层socket
	for (int i=0;i<MAX_SOCKET;i++) {
		struct socket *s = &ss->slot[i];
		s->type = SOCKET_TYPE_INVALID;
	}
	ss->alloc_id = 0;
	ss->callback = callback;
	ss->pool = socket_buffer_pool_create();

	SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sock)
	{
		fprintf(stderr, "socket-server: create socket failed with error:%d.\n", WSAGetLastError());
		return NULL;
	}
	
	// 初始化扩展函数指针
	ss->pfn_connectex				= Get_ConnectEx_FuncPtr(sock);
	ss->pfn_acceptex				= Get_AcceptEx_FuncPtr(sock);
	ss->pfn_get_acceptex_sockaddrs	= Get_GetAcceptExSockaddrs_FuncPtr(sock);
	ss->pfn_disconnectex			= Get_DisconnectEx_FuncPtr(sock);

	closesocket(sock);

	ss->thread = thread;
	ss->handles = (HANDLE*)malloc(sizeof(HANDLE) * thread);

	
	
	return ss;
}

void socket_server_start_socket_thread(struct socket_server* ss)
{
	// 启动工作线程
	for (int i=0; i<ss->thread; ++i)
	{
		ss->handles[i] = (HANDLE)_beginthreadex(NULL, 0, worker_thread, (void*)ss, 0, NULL);
	}
}

void socket_server_release(struct socket_server* ss)
{
	//int i;
	//// 关闭socket
	//for (i=0;i<MAX_SOCKET;i++) {
	//	struct socket *s = &ss->slot[i];
	//	if (s->type != SOCKET_TYPE_RESERVE) {
	//		do_close(s);
	//	}
	//}

	iocp_release(ss->event_fd);		// 关闭完成端口
	socket_buffer_pool_release(ss->pool);		// 销毁socket缓冲池
	free(ss->slot);
	free(ss);

	WSACleanup();
}

void socket_server_exit(struct socket_server* ss)
{
	// 关闭socket
	for (int i=0;i<MAX_SOCKET;i++) {
		struct socket *s = &ss->slot[i];
		if (s->type != SOCKET_TYPE_RESERVE) {
			do_close(s);
		}
	}

	for(int i=0; i<ss->thread; ++i)
		PostQueuedCompletionStatus(ss->event_fd, CMD_EXIT, 0, NULL);
}

void socket_server_wait_for_exit(struct socket_server* ss)
{
	//assert(WaitForMultipleObjects(ss->thread, ss->handles, TRUE, INFINITE) == WAIT_OBJECT_0);

	WaitForMultipleObjects(ss->thread, ss->handles, TRUE, INFINITE);
	int n = GetLastError();

	for (int i=0; i<ss->thread; ++i)
		CloseHandle(ss->handles[i]);
}

int socket_server_listen(struct socket_server* ss, uintptr_t opaque, const char * addr, int port, int backlog)
{
	int fd = do_listen(addr, port, backlog);
	if (fd < 0) {
		return -1;
	}

	int id = reserve_id(ss);

	struct socket * s = &ss->slot[id % MAX_SOCKET];
	s->fd = fd;
	s->id = id;
	s->opaque = opaque;
	s->type = SOCKET_TYPE_LISTEN;

	// 监听套接字与完成端口绑定
	if(CreateIoCompletionPort((HANDLE)fd, ss->event_fd, (ULONG_PTR)s, 0))
	{
		for(int i = 0; i < backlog; i++)
			PostQueuedCompletionStatus(ss->event_fd, CMD_ACCEPT, (ULONG_PTR)s, NULL);		// 通知完成端口投递异步accept请求,即OP_ACCEPT
	}
	else
		return -1;

	return id;
}

void socket_server_start(struct socket_server* ss, uintptr_t opaque, int id)
{
}

int socket_server_bind(struct socket_server *ss, uintptr_t opaque, int fd) {
	int id = reserve_id(ss);
	struct socket * s = &ss->slot[id % MAX_SOCKET];
	s->fd = fd;
	s->id = id;
	s->opaque = opaque;
	s->type = SOCKET_TYPE_BIND;

	CreateIoCompletionPort((HANDLE)fd, ss->event_fd, (ULONG_PTR)s, 0);

	return id;
}

int socket_server_connect(struct socket_server *ss, uintptr_t opaque, const char * addr, int port) {
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int id = reserve_id(ss);

	struct socket * s = &ss->slot[id % MAX_SOCKET];
	s->fd = sock;
	s->id = id;
	s->opaque = opaque;
	s->type = SOCKET_TYPE_CONNECTING;

	CreateIoCompletionPort((HANDLE)sock, ss->event_fd, (ULONG_PTR)s, 0);


	struct sockaddr_in local_addr;
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;  
	local_addr.sin_addr.s_addr = INADDR_ANY;  
	local_addr.sin_port = htons(0);

	int result =bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));
	socket_buffer*	sb	= socket_buffer_pool_get(ss->pool);

	struct sockaddr_in remote_addr;
	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;  
	remote_addr.sin_addr.s_addr = inet_addr(addr);  
	remote_addr.sin_port = htons(port);

	post_connect(ss->pfn_connectex, sock, &remote_addr, sb);

	return id;
}

int socket_server_block_connect(struct socket_server *ss, uintptr_t opaque, const char * addr, int port) {
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int id = reserve_id(ss);

	struct socket* s = &ss->slot[id % MAX_SOCKET];
	s->fd = sock;
	s->id = id;

	struct sockaddr_in peeraddr;
	memset(&peeraddr, 0, sizeof(peeraddr));
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_port = htons(port);
	peeraddr.sin_addr.s_addr = inet_addr(addr);

	int status = connect(sock, (struct sockaddr*)&peeraddr, sizeof(peeraddr));
	if (status != 0)
	{
		closesocket(sock);
		ss->slot[id % MAX_SOCKET].type = SOCKET_TYPE_INVALID;		// 归还socket
		return -1;
	}

	keep_alive(sock, TRUE);
	s->type = SOCKET_TYPE_CONNECTED;

	CreateIoCompletionPort((HANDLE)sock, ss->event_fd, (ULONG_PTR)s, 0);

	struct socket_buffer* sb = socket_buffer_pool_get(ss->pool);
	do_receive(ss, s, sb);

	return id;
}



int64_t socket_server_send(struct socket_server *ss, int id, const void * buffer, int sz) {
	struct socket * s = &ss->slot[id % MAX_SOCKET];
	if (s->id != id || s->type == SOCKET_TYPE_INVALID) {
		return -1;
	}
	assert(s->type != SOCKET_TYPE_RESERVE);

	assert(buffer && sz > 0);
	do_send(ss, s, (const char*)buffer, sz);

	/*
	ASSERT(pBuffer && iLength > 0);

	int result				= NO_ERROR;
	TSocketObj* pSocketObj	= FindSocketObj(dwConnID);

	if(!TSocketObj::IsValid(pSocketObj))
		result = ERROR_OBJECT_NOT_FOUND;
	else
	{
		result = DoSend(dwConnID, pSocketObj, pBuffer, iLength);

		if(result != NO_ERROR)
			CheckError(dwConnID, SO_SEND, result);
	}
		
	if(result != NO_ERROR)
		::SetLastError(result);

	return (result == NO_ERROR);
	*/

	return 0;
}



void socket_server_close(struct socket_server *ss, uintptr_t opaque, int id) {
	/*
	struct socket * s = &ss->slot[id % MAX_SOCKET];
	if (s->type == SOCKET_TYPE_INVALID || s->id != id) {
		return;
	}
	s->type = SOCKET_TYPE_HALFCLOSE;

	::PostQueuedCompletionStatus(ss->event_fd, IOCP_SI_DISCONNECT, (ULONG_PTR)s, NULL);
	*/

	//m_pfnDisconnectEx(pSocketObj->socket, nullptr, 0, 0);
	struct socket * s = &ss->slot[id % MAX_SOCKET];
	do_close(s);
}
