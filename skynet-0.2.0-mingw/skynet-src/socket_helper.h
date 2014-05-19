#ifndef _SOCKET_HELPER_H_
#define _SOCKET_HELPER_H_

//#pragma comment(lib, "ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#ifndef WSAID_ACCEPTEX
#define WSAID_ACCEPTEX \
	{0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#endif
#ifndef WSAID_CONNECTEX
#define WSAID_CONNECTEX \
	{0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}
#endif
#ifndef WSAID_GETACCEPTEXSOCKADDRS
#define WSAID_GETACCEPTEXSOCKADDRS \
	{0xb5367df2,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#endif
#ifndef WSAID_DISCONNECTEX
#define WSAID_DISCONNECTEX \
	{0x7fda2e11,0x8630,0x436f,{0xa0, 0x31, 0xf5, 0x36, 0xa6, 0xee, 0xc1, 0x57}}
#endif

#ifndef SO_UPDATE_CONNECT_CONTEXT
#define SO_UPDATE_CONNECT_CONTEXT   0x7010
#endif

typedef BOOL (WINAPI *AcceptExPtr)(SOCKET, SOCKET, PVOID, DWORD, DWORD, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *ConnectExPtr)(SOCKET, const struct sockaddr *, int, PVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef void (WINAPI *GetAcceptExSockaddrsPtr)(PVOID, DWORD, DWORD, DWORD, LPSOCKADDR *, LPINT, LPSOCKADDR *, LPINT);
typedef BOOL (PASCAL FAR * DisconnectExPtr)(SOCKET, LPOVERLAPPED, DWORD, DWORD);

struct win32_extension_fns
{
	AcceptExPtr AcceptEx;
	ConnectExPtr ConnectEx;
	DisconnectExPtr DisconnectEx;
	GetAcceptExSockaddrsPtr GetAcceptExSockaddrs;
};

enum socket_operation
{
	OP_UNKNOWN	= 0,	// Unknown
	OP_ACCEPT	= 1,	// Acccept
	OP_CONNECT	= 2,	// Connect
	OP_SEND		= 3,	// Send
	OP_RECEIVE	= 4,	// Receive
};

/* 数据缓冲区基础结构 */
struct socket_buffer
{
	OVERLAPPED			overlapped;
	WSABUF				buffer;
	enum socket_operation	operation;
	SOCKET				sock;
};

int post_connect(SOCKET soClient, SOCKADDR_IN* soAddrIN, struct socket_buffer* sb);
int post_accept(SOCKET soListen, SOCKET soClient, struct socket_buffer* sb);
int post_receive(SOCKET sock, struct socket_buffer* sb);
int post_send(SOCKET sock, struct socket_buffer* sb);

int close_socket(SOCKET sock);

int set_linger(SOCKET sock, USHORT l_onoff, USHORT l_linger);
int set_reuse_address(SOCKET sock, BOOL reuse);
int update_connect_context(SOCKET sock);
int update_accept_context(SOCKET sock, SOCKET listenfd);

int keep_alive(SOCKET sock, BOOL bKeepAlive);

int socket_startup();
int socket_cleanup();

const struct win32_extension_fns* get_win32_extension_fns();

#endif // _SOCKET_HELPER_H_
