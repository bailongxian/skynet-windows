#include "socket_buffer_pool.h"
#include <stdlib.h>
#include <assert.h>

struct socket_buffer_pool
{
	int cap;
	int head;
	int tail;
	volatile long lock;
	struct socket_buffer** queue;
	CRITICAL_SECTION cs;
};

//EnterCriticalSection()

#define LOCK(pool) EnterCriticalSection(&pool->cs);
#define UNLOCK(pool) LeaveCriticalSection(&pool->cs);

//#define LOCK(pool) while (InterlockedCompareExchange(&(pool)->lock,1, 1)) {}
//#define UNLOCK(pool) InterlockedExchange(&(pool)->lock, 0);

#include <stdio.h>
static struct socket_buffer* create_buffer()
{
	//printf("create buffer\n");
	struct socket_buffer* sb = (struct socket_buffer*)malloc(sizeof(socket_buffer)+DEFAULT_SOCKET_BUFFER_SIZE);
	assert(sb);
	memset(sb, 0, sizeof(*sb));
	sb->buffer.buf = ((char*)sb) + sizeof(struct socket_buffer);

	return sb;
}

static void free_buffer(struct socket_buffer* sb)
{
	//printf("free buffer\n");
	assert(sb);
	free(sb);
}

struct socket_buffer_pool* socket_buffer_pool_create()
{
	struct socket_buffer_pool* pool = (struct socket_buffer_pool*)malloc(sizeof(*pool));
	pool->cap = DEFAULT_POOL_SIZE;
	pool->head = 0;
	pool->tail = 0;
	pool->lock = 0;
	pool->queue = (struct socket_buffer**)malloc(sizeof(struct socket_buffer*) * pool->cap);
	InitializeCriticalSection(&pool->cs);
	for (int i=0; i<DEFAULT_POOL_SIZE; ++i)
	{
		socket_buffer_pool_put(pool, create_buffer());
	}
	
	return pool;
}

static void _release(struct socket_buffer_pool* pool)
{
	free(pool->queue);
	free(pool);
}

int socket_buffer_pool_length(struct socket_buffer_pool* pool)
{
	int head, tail,cap;

	//LOCK(pool)
	head = pool->head;
	tail = pool->tail;
	cap = pool->cap;
	//UNLOCK(pool)
	
	if (head <= tail) {
		return tail - head;
	}
	return tail + cap - head;
}

struct socket_buffer* socket_buffer_pool_get(struct socket_buffer_pool* pool)
{
	struct socket_buffer* sb = NULL;
	LOCK(pool)

	if (pool->head != pool->tail)
	{
		sb = pool->queue[pool->head];
		if ( ++pool->head >= pool->cap)
		{
			pool->head = 0;
		}
	}
	else
		sb = create_buffer();

	sb->buffer.len = DEFAULT_SOCKET_BUFFER_SIZE;

	UNLOCK(pool)
	return sb;
}

static void expand_queue(struct socket_buffer_pool *pool)
{
	struct socket_buffer **new_queue = (struct socket_buffer**)malloc(sizeof(struct socket_buffer*) * pool->cap * 2);
	int i;
	for (i=0;i<pool->cap;i++) {
		new_queue[i] = pool->queue[(pool->head + i) % pool->cap];
	}
	pool->head = 0;
	pool->tail = pool->cap;
	pool->cap *= 2;
	
	free(pool->queue);
	pool->queue = new_queue;
}


static void 
_pushhead(struct socket_buffer_pool* pool, struct socket_buffer* sb) {
	int head = pool->head - 1;
	if (head < 0) {
		head = pool->cap - 1;
	}
	if (head == pool->tail) {
		expand_queue(pool);
		--pool->tail;
		head = pool->cap - 1;
	}

	pool->queue[head] = sb;
	pool->head = head;
}

void socket_buffer_pool_put(struct socket_buffer_pool* pool, struct socket_buffer* sb) {
	assert(sb);
	LOCK(pool)
	
	// 队列尾部插入
	pool->queue[pool->tail] = sb;
	if (++ pool->tail >= pool->cap)
	{
		pool->tail = 0;
	}

	// 队列满
	if (pool->head == pool->tail)
	{
		expand_queue(pool);
	}
	
	//printf("%d\n", socket_buffer_pool_length(pool));
	if (socket_buffer_pool_length(pool)>=MAX_POOL_SIZE)
	{
		while (socket_buffer_pool_length(pool) > DEFAULT_POOL_SIZE)
		{
			struct socket_buffer* sb = pool->queue[pool->head];

			if ( ++pool->head >= pool->cap)
			{
				pool->head = 0;
			}
			
			free_buffer(sb);
		}

	}
	
	UNLOCK(pool)
	
}

void socket_buffer_pool_release(struct socket_buffer_pool* pool)
{
	struct socket_buffer* sb;
	LOCK(pool)
	while (pool->head != pool->tail)
	{
		sb = pool->queue[pool->head];
		if ( ++pool->head >= pool->cap)
		{
			pool->head = 0;
		}
		free_buffer(sb);
	}
	UNLOCK(pool)

	DeleteCriticalSection(&pool->cs);
	_release(pool);
}