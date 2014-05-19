#ifndef _SOCKET_BUFFER_POOL_H_
#define _SOCKET_BUFFER_POOL_H_

#include "socket_helper.h"

#define DEFAULT_POOL_SIZE			64
#define MAX_POOL_SIZE				4096
#define DEFAULT_SOCKET_BUFFER_SIZE			(4096-sizeof(struct socket_buffer))

struct socket_buffer_pool;

struct socket_buffer_pool* socket_buffer_pool_create();
void socket_buffer_pool_release(struct socket_buffer_pool* pool);
int socket_buffer_pool_length(struct socket_buffer_pool* pool);
struct socket_buffer* socket_buffer_pool_get(struct socket_buffer_pool* pool);
void socket_buffer_pool_put(struct socket_buffer_pool* pool, struct socket_buffer* sb);

#endif // _SOCKET_BUFFER_POOL_H_

