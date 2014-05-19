#ifndef skynet_socket_server_h
#define skynet_socket_server_h

#include "skynet_macro.h"
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

#ifdef __cplusplus
extern "C"
{
#endif

// socket消息
#define SOCKET_DATA			0
#define SOCKET_CLOSE		1
#define SOCKET_OPEN			2
#define SOCKET_ACCEPT		3
#define SOCKET_ERROR2		4
#define SOCKET_EXIT			5
#define SOCKET_SENT			6			// 新增，发送完毕消息

struct socket {
	int fd;
	int id;
	volatile long type;
	uintptr_t opaque;
};

typedef int (*socket_result_callback)(int, struct socket_message*);

struct socket_message {
	int id;
	uintptr_t opaque;
	int ud;	// for accept, ud is new accept id ; for data, ud is size of data 
	char * data;
};

static inline int get_number_of_processors()
{
	SYSTEM_INFO systemInfo = {0};
	GetSystemInfo(&systemInfo);

	return systemInfo.dwNumberOfProcessors;
}

static inline int default_callback(int type, struct socket_message* result)
{
	return 0;
}

SKYNET_API struct socket_server * socket_server_create(socket_result_callback callback=default_callback, int thread= get_number_of_processors());
SKYNET_API void socket_server_release(struct socket_server *);
SKYNET_API void socket_server_start_socket_thread(struct socket_server*);

SKYNET_API void socket_server_exit(struct socket_server *);
SKYNET_API void socket_server_wait_for_exit(struct socket_server *);
SKYNET_API void socket_server_close(struct socket_server *, uintptr_t opaque, int id);
SKYNET_API void socket_server_start(struct socket_server *, uintptr_t opaque, int id);

SKYNET_API int64_t socket_server_send(struct socket_server *, int id, const void * buffer, int sz);

SKYNET_API int socket_server_listen(struct socket_server *, uintptr_t opaque, const char * addr, int port, int backlog/*=SOMAXCONN*/);
SKYNET_API int socket_server_connect(struct socket_server *, uintptr_t opaque, const char * addr, int port);
SKYNET_API int socket_server_bind(struct socket_server *, uintptr_t opaque, int fd);

SKYNET_API int socket_server_block_connect(struct socket_server *, uintptr_t opaque, const char * addr, int port);

#ifdef __cplusplus
}
#endif


#endif
