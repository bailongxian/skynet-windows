#ifndef _SOCKET_HELPER_H_
#define _SOCKET_HELPER_H_

#pragma comment(lib, "ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ws2tcpip.h>
#include <mswsock.h>

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

void* GetExtensionFuncPtr(SOCKET sock, GUID guid);
LPFN_ACCEPTEX Get_AcceptEx_FuncPtr(SOCKET sock);
LPFN_GETACCEPTEXSOCKADDRS Get_GetAcceptExSockaddrs_FuncPtr(SOCKET sock);
LPFN_CONNECTEX Get_ConnectEx_FuncPtr(SOCKET sock);
LPFN_DISCONNECTEX Get_DisconnectEx_FuncPtr	(SOCKET sock);

int post_connect(LPFN_CONNECTEX pfnConnectEx, SOCKET soClient, SOCKADDR_IN* soAddrIN, struct socket_buffer* pBufferObj);
int post_accept(LPFN_ACCEPTEX pfnAcceptEx, SOCKET soListen, SOCKET soClient, struct socket_buffer* pBufferObj);
int post_receive(SOCKET sock, struct socket_buffer* pBufferObj);
int post_send(SOCKET sock, struct socket_buffer* pBufferObj);

int close_socket(SOCKET sock);

int set_linger(SOCKET sock, USHORT l_onoff, USHORT l_linger);
int set_reuse_address(SOCKET sock, BOOL reuse);
int update_connect_context(SOCKET sock);
int update_accept_context(SOCKET sock, SOCKET listenfd);

int keep_alive(SOCKET sock, BOOL bKeepAlive);
int keep_alive_vals(SOCKET sock, u_long onoff, u_long time, u_long interval);

#endif // _SOCKET_HELPER_H_
