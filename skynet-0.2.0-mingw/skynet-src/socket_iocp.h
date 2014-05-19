#ifndef _SOCKET_IOCP_H_
#define _SOCKET_IOCP_H_

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef HANDLE iocp_fd;

static iocp_fd iocp_create()
{
	return CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
}

static int iocp_invalid(iocp_fd fd)
{
	return fd == NULL;
}

static void iocp_release(iocp_fd fd)
{
	CloseHandle(fd);
}

static int iocp_post_status(iocp_fd fd, DWORD bytes, ULONG_PTR key, OVERLAPPED* ol)
{
	int r;
	r = PostQueuedCompletionStatus(fd, bytes, key, ol);
	return (r==0 ? -1 : 0);
}

static int iocp_get_status(iocp_fd fd, DWORD* bytes, ULONG_PTR* key, OVERLAPPED** ol, DWORD milli_seconds)
{
	return GetQueuedCompletionStatus(fd, bytes, key, ol, milli_seconds) ? 0 : -1;
}

static int iocp_add(iocp_fd fd, HANDLE h, ULONG_PTR key)
{
	return (CreateIoCompletionPort(h, fd, key, 0) == fd) ? 0 : -1;
}

#endif // _SOCKET_IOCP_H_
