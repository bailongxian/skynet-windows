#ifndef skynet_socket_h
#define skynet_socket_h

#include "skynet_macro.h"
#ifdef __cplusplus
extern "C"
{
#endif

struct skynet_context;
struct socket_message;

#define SKYNET_SOCKET_TYPE_DATA 1
#define SKYNET_SOCKET_TYPE_CONNECT 2
#define SKYNET_SOCKET_TYPE_CLOSE 3
#define SKYNET_SOCKET_TYPE_ACCEPT 4
#define SKYNET_SOCKET_TYPE_ERROR 5

struct skynet_socket_message {
	int type;
	int id;
	int ud;
	char * buffer;
};

SKYNET_API void skynet_socket_start_socket_thread();

SKYNET_API void skynet_socket_init();
SKYNET_API void skynet_socket_exit();
SKYNET_API void skynet_socket_free();
//int skynet_socket_poll();
int socket_callback(int type, struct socket_message* result);

SKYNET_API int skynet_socket_send(struct skynet_context *ctx, int id, void *buffer, int sz);
//void skynet_socket_send_lowpriority(struct skynet_context *ctx, int id, void *buffer, int sz);
SKYNET_API int skynet_socket_listen(struct skynet_context *ctx, const char *host, int port, int backlog);
SKYNET_API int skynet_socket_connect(struct skynet_context *ctx, const char *host, int port);
SKYNET_API int skynet_socket_block_connect(struct skynet_context *ctx, const char *host, int port);
SKYNET_API int skynet_socket_bind(struct skynet_context *ctx, int fd);
SKYNET_API void skynet_socket_close(struct skynet_context *ctx, int id);
SKYNET_API void skynet_socket_start(struct skynet_context *ctx, int id);

#ifdef __cplusplus
}
#endif

#endif
