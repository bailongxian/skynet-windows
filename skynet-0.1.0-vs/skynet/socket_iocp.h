#ifndef _SOCKET_IOCP_H_
#define _SOCKET_IOCP_H_

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef HANDLE iocp_fd;

static iocp_fd iocp_create()
{
	return CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
}

static /*bool*/int iocp_invalid(iocp_fd fd)
{
	return fd == NULL;
}

static void iocp_release(iocp_fd fd)
{
	CloseHandle(fd);
}


#endif // _SOCKET_IOCP_H_
