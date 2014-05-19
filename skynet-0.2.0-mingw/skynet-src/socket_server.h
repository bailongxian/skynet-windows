#ifndef skynet_socket_server_h
#define skynet_socket_server_h

#include <stdint.h>

#define SOCKET_DATA 0
#define SOCKET_CLOSE 1
#define SOCKET_OPEN 2
#define SOCKET_ACCEPT 3
#define SOCKET_ERROR2 4
#define SOCKET_EXIT 5
#define SOCKET_SENT 6		// 新增，发送完毕

struct socket_server;

struct socket_message {
	int id;
	uintptr_t opaque;
	int ud;	// for accept, ud is new accepted id ; for data, ud is size of data 
	char * data;
};

typedef void (*threadinit_callback)();
typedef int (*socket_result_callback)(int, struct socket_message*);

struct socket_server * socket_server_create();
void socket_server_set_callback(struct socket_server *, socket_result_callback cb, threadinit_callback initcb);
void socket_server_start_thread(struct socket_server *);
void socket_server_release(struct socket_server *);
int socket_server_poll(struct socket_server *, struct socket_message *result, int *more);

void socket_server_exit(struct socket_server *);
// 新增
void socket_server_wait_for_exit(struct socket_server *);
void socket_server_close(struct socket_server *, uintptr_t opaque, int id);
void socket_server_start(struct socket_server *, uintptr_t opaque, int id);

// return -1 when error
int64_t socket_server_send(struct socket_server *, int id, const void * buffer, int sz);
void socket_server_send_lowpriority(struct socket_server *, int id, const void * buffer, int sz);

// ctrl command below returns id
int socket_server_listen(struct socket_server *, uintptr_t opaque, const char * addr, int port, int backlog);
int socket_server_connect(struct socket_server *, uintptr_t opaque, const char * addr, int port);
int socket_server_bind(struct socket_server *, uintptr_t opaque, int fd);

int socket_server_block_connect(struct socket_server *, uintptr_t opaque, const char * addr, int port);

#endif
