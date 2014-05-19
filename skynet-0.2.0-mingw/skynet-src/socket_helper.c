#include "socket_helper.h"
#include <assert.h>
#include <stdlib.h>

static void *
get_extension_function(SOCKET s, const GUID *which_fn)
{
	void *ptr = NULL;
	DWORD bytes=0;
	WSAIoctl(s,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
	    (GUID*)which_fn,
		sizeof(*which_fn),
	    &ptr,
		sizeof(ptr),
	    &bytes,
		NULL,
		NULL);

	return ptr;
}

static struct win32_extension_fns the_extension_fns;
//static int extension_fns_initialized = 0;

const struct win32_extension_fns* get_win32_extension_fns()
{
	return &the_extension_fns;
}


static void init_extension_functions(struct win32_extension_fns *ext)
{
	const GUID acceptex = WSAID_ACCEPTEX;
	const GUID connectex = WSAID_CONNECTEX;
	const GUID disconnectex = WSAID_DISCONNECTEX;
	const GUID getacceptexsockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
		return;
	ext->AcceptEx = (AcceptExPtr)get_extension_function(s, &acceptex);
	ext->ConnectEx = (ConnectExPtr)get_extension_function(s, &connectex);
	ext->DisconnectEx = (DisconnectExPtr)get_extension_function(s, &disconnectex);
	ext->GetAcceptExSockaddrs = (GetAcceptExSockaddrsPtr)get_extension_function(s,
		&getacceptexsockaddrs);
	closesocket(s);
}

int socket_startup()
{
	WSADATA wsaData;

	// Æô¶¯WinSock
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0)
		return -1;
	init_extension_functions(&the_extension_fns);
	return 0;
}

int socket_cleanup()
{
	return WSACleanup();
}

int close_socket(SOCKET sock)
{
	shutdown(sock, SD_SEND);
	set_linger(sock, 1, 0);
	return closesocket(sock);
}

int update_connect_context(SOCKET sock)
{
	return setsockopt(sock, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
}

int update_accept_context(SOCKET sock, SOCKET listenfd)
{
	return setsockopt(sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char*)&listenfd, sizeof(SOCKET));
}

int post_accept(SOCKET soListen, SOCKET soClient, struct socket_buffer* sb)
{
	int result				= NO_ERROR;
	sb->sock		= soClient;
	sb->operation	= OP_ACCEPT;

	AcceptExPtr pfnAcceptEx = the_extension_fns.AcceptEx;

	if(!pfnAcceptEx(
		soListen,
		sb->sock,
		sb->buffer.buf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		NULL,
		&sb->overlapped))
	{
		result = WSAGetLastError();
		if(result == WSA_IO_PENDING)
			result = NO_ERROR;
	}

	return result;
}

int post_connect(SOCKET soClient, SOCKADDR_IN* soAddrIN, struct socket_buffer* sb)
{
	int result				= NO_ERROR;
	sb->sock		= soClient;
	sb->operation	= OP_CONNECT;

	ConnectExPtr pfnConnectEx = the_extension_fns.ConnectEx;
	if(!pfnConnectEx(
		soClient,
		(SOCKADDR*)soAddrIN,
		sizeof(SOCKADDR_IN),
		NULL,
		0,
		NULL,
		&sb->overlapped
		)
		)
	{
		result = WSAGetLastError();
		if(result == WSA_IO_PENDING)
			result = NO_ERROR;
	}

	return result;
}


int post_receive(SOCKET sock, struct socket_buffer* sb)
{
	int result				= NO_ERROR;
	sb->operation	= OP_RECEIVE;
	DWORD dwBytes, dwFlag	= 0; 

	if(WSARecv(
					sock,
					&sb->buffer,
					1,
					&dwBytes,
					&dwFlag,
					&sb->overlapped,
					NULL
				) == SOCKET_ERROR)
	{
		result = WSAGetLastError();
		if(result == WSA_IO_PENDING)
			result = NO_ERROR;
	}

	return result;
}

int post_send(SOCKET sock, struct socket_buffer* sb)
{
	int result				= NO_ERROR;
	sb->operation	= OP_SEND;
	DWORD dwBytes;

	if(WSASend(
					sock,
					&sb->buffer,
					1,
					&dwBytes,
					0,
					&sb->overlapped,
					NULL
				) == SOCKET_ERROR)
	{
		result = WSAGetLastError();
		if(result == WSA_IO_PENDING)
			result = NO_ERROR;
	}

	return result;
}



int set_linger(SOCKET sock, USHORT l_onoff, USHORT l_linger)
{
	struct linger l = {l_onoff, l_linger};
	return setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(struct linger));
}

int set_reuse_address(SOCKET sock, BOOL reuse)
{
	return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(BOOL));
}


int keep_alive(SOCKET sock, BOOL bKeepAlive)
{
	return setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (CHAR*)&bKeepAlive, sizeof(BOOL));
}
