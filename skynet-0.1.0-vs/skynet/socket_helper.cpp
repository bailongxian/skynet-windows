#include "socket_helper.h"

#include <mstcpip.h>
#include <assert.h>
#include <stdlib.h>


void* GetExtensionFuncPtr(SOCKET sock, GUID guid)
{
	DWORD dwBytes;
	void* pfn = NULL;

	WSAIoctl(
		sock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid,
		sizeof(guid),
		&pfn,
		sizeof(pfn),
		&dwBytes,
		NULL,
		NULL
		);

	return pfn;
}

LPFN_ACCEPTEX Get_AcceptEx_FuncPtr(SOCKET sock)
{
	GUID guid = WSAID_ACCEPTEX;
	return (LPFN_ACCEPTEX)GetExtensionFuncPtr(sock, guid);
}

LPFN_GETACCEPTEXSOCKADDRS Get_GetAcceptExSockaddrs_FuncPtr(SOCKET sock)
{
	GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
	return (LPFN_GETACCEPTEXSOCKADDRS)GetExtensionFuncPtr(sock, guid);
}

LPFN_CONNECTEX Get_ConnectEx_FuncPtr(SOCKET sock)
{
	GUID guid = WSAID_CONNECTEX;
	return (LPFN_CONNECTEX)GetExtensionFuncPtr(sock, guid);
}

LPFN_DISCONNECTEX Get_DisconnectEx_FuncPtr	(SOCKET sock)
{
	GUID guid = WSAID_DISCONNECTEX;
	return (LPFN_DISCONNECTEX)GetExtensionFuncPtr(sock, guid);
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

int post_accept(LPFN_ACCEPTEX pfnAcceptEx, SOCKET soListen, SOCKET soClient, struct socket_buffer* pBufferObj)
{
	int result				= NO_ERROR;
	pBufferObj->sock		= soClient;
	pBufferObj->operation	= OP_ACCEPT;

	if(!pfnAcceptEx(
		soListen,
		pBufferObj->sock,
		pBufferObj->buffer.buf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		NULL,
		&pBufferObj->overlapped))
	{
		result = WSAGetLastError();
		if(result == WSA_IO_PENDING)
			result = NO_ERROR;
	}

	return result;
}

int post_connect(LPFN_CONNECTEX pfnConnectEx, SOCKET soClient, SOCKADDR_IN* soAddrIN, struct socket_buffer* pBufferObj)
{
	int result				= NO_ERROR;
	pBufferObj->sock		= soClient;
	pBufferObj->operation	= OP_CONNECT;

	if(!pfnConnectEx(
		soClient,
		(SOCKADDR*)soAddrIN,
		sizeof(SOCKADDR_IN),
		NULL,
		0,
		NULL,
		&pBufferObj->overlapped
		)
		)
	{
		result = WSAGetLastError();
		if(result == WSA_IO_PENDING)
			result = NO_ERROR;
	}

	return result;
}


int post_receive(SOCKET sock, struct socket_buffer* pBufferObj)
{
	int result				= NO_ERROR;
	pBufferObj->operation	= OP_RECEIVE;
	DWORD dwBytes, dwFlag	= 0; 

	if(WSARecv(
					sock,
					&pBufferObj->buffer,
					1,
					&dwBytes,
					&dwFlag,
					&pBufferObj->overlapped,
					NULL
				) == SOCKET_ERROR)
	{
		result = WSAGetLastError();
		if(result == WSA_IO_PENDING)
			result = NO_ERROR;
	}

	return result;
}

int post_send(SOCKET sock, struct socket_buffer* pBufferObj)
{
	int result				= NO_ERROR;
	pBufferObj->operation	= OP_SEND;
	DWORD dwBytes;

	if(WSASend(
					sock,
					&pBufferObj->buffer,
					1,
					&dwBytes,
					0,
					&pBufferObj->overlapped,
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
	linger l = {l_onoff, l_linger};
	return setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(linger));
}

int set_reuse_address(SOCKET sock, BOOL reuse)
{
	return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(BOOL));
}


int keep_alive(SOCKET sock, BOOL bKeepAlive)
{
	return setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (CHAR*)&bKeepAlive, sizeof(BOOL));
}

int keep_alive_vals(SOCKET sock, u_long onoff, u_long time, u_long interval)
{
	int result		 = NO_ERROR;
	tcp_keepalive in = {onoff, time, interval};
	DWORD dwBytes;

	if(WSAIoctl	(
		sock, 
		SIO_KEEPALIVE_VALS, 
		(LPVOID)&in, 
		sizeof(in), 
		NULL, 
		0, 
		&dwBytes, 
		NULL, 
		NULL
		) == SOCKET_ERROR)
	{
		result = WSAGetLastError();
		if(result == WSAEWOULDBLOCK)
			result = NO_ERROR;
	}

	return result;
}
