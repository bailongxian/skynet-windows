#include "skynet.h"
#include "skynet_server.h"
#include "skynet_imp.h"
#include "skynet_mq.h"
#include "skynet_handle.h"
#include "skynet_module.h"
#include "skynet_timer.h"
#include "skynet_harbor.h"
#include "skynet_monitor.h"
#include "skynet_socket.h"

//#include <pthread.h>
//#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct monitor {
	int count;
	struct skynet_monitor ** m;
	CONDITION_VARIABLE cond;
	CRITICAL_SECTION cs;
	//pthread_cond_t cond;
	//pthread_mutex_t mutex;
	int sleep;
};

struct worker_parm {
	struct monitor *m;
	int id;
};

#define CHECK_ABORT if (skynet_context_total()==0) break;


static HANDLE
create_thread(unsigned (__stdcall * start_routine)(void *), void *arg) {
	uintptr_t  hThread = _beginthreadex(NULL, 0, start_routine, arg, 0, NULL);
	if (hThread == (uintptr_t)-1)
	{
		fprintf(stderr, "Create thread failed");
		exit(1);
	}
	return (HANDLE)hThread;
}

static void
wakeup(struct monitor *m, int busy) {
	if (m->sleep >= m->count - busy) {
		// signal sleep worker, "spurious wakeup" is harmless
		//pthread_cond_signal(&m->cond);
		WakeConditionVariable(&m->cond);
	}
}

struct monitor* M = NULL;
void wakeup_worker()
{
	wakeup(M, 0);
}

/*
static void *
_socket(void *p) {
	struct monitor * m = p;
	for (;;) {
		int r = skynet_socket_poll();
		if (r==0)
			break;
		if (r<0) {
			CHECK_ABORT
			continue;
		}
		wakeup(m,0);
	}
	return NULL;
}*/

static void
free_monitor(struct monitor *m) {
	int i;
	int n = m->count;
	for (i=0;i<n;i++) {
		skynet_monitor_delete(m->m[i]);
	}
	//pthread_mutex_destroy(&m->mutex);
	//pthread_cond_destroy(&m->cond);
	DeleteCriticalSection(&m->cs);
	skynet_free(m->m);
	skynet_free(m);
}

static unsigned __stdcall
_monitor(void *p) {
	struct monitor * m = (struct monitor*)p;
	int i;
	int n = m->count;
	for (;;) {
		CHECK_ABORT
		for (i=0;i<n;i++) {
			skynet_monitor_check(m->m[i]);
		}
		for (i=0;i<5;i++) {
			CHECK_ABORT
			//sleep(1);
			Sleep(1000);
		}
	}

	return NULL;
}

static unsigned __stdcall
_timer(void *p) {
	struct monitor * m = (struct monitor*)p;
	for (;;) {
		skynet_updatetime();
		CHECK_ABORT
		wakeup(m,m->count-1);
		//usleep(2500);
		Sleep(2);
	}
	// wakeup socket thread
	skynet_socket_exit();
	// wakeup all worker thread
//	pthread_cond_broadcast(&m->cond);
	WakeAllConditionVariable(&m->cond);
	return 0;
}

static unsigned __stdcall
_worker(void *p) {
	struct worker_parm *wp = (struct worker_parm*)p;
	int id = wp->id;
	struct monitor *m = wp->m;
	struct skynet_monitor *sm = m->m[id];
	for (;;) {
		if (skynet_context_message_dispatch(sm)) {
			CHECK_ABORT
			EnterCriticalSection(&m->cs);
			++ m->sleep;

			SleepConditionVariableCS(&m->cond, &m->cs, INFINITE);
			-- m->sleep;
			LeaveCriticalSection(&m->cs);
			/*
			if (pthread_mutex_lock(&m->mutex) == 0) {
				++ m->sleep;
				// "spurious wakeup" is harmless,
				// because skynet_context_message_dispatch() can be call at any time.
				pthread_cond_wait(&m->cond, &m->mutex);
				-- m->sleep;
				if (pthread_mutex_unlock(&m->mutex)) {
					fprintf(stderr, "unlock mutex error");
					exit(1);
				}
			}*/

		} 
	}
	return 0;
}

static void
_start(int thread) {

	//unsigned pid[thread+3];

	HANDLE* threads = (HANDLE*)skynet_malloc(sizeof(HANDLE) * (thread+2));

	struct monitor *m = (struct monitor*)skynet_malloc(sizeof(*m));
	M = m;
	memset(m, 0, sizeof(*m));
	m->count = thread;
	m->sleep = 0;

	m->m = (struct skynet_monitor**)skynet_malloc(thread * sizeof(struct skynet_monitor *));
	int i;
	for (i=0;i<thread;i++) {
		m->m[i] = skynet_monitor_new();
	}
	/*
	if (pthread_mutex_init(&m->mutex, NULL)) {
		fprintf(stderr, "Init mutex error");
		exit(1);
	}
	if (pthread_cond_init(&m->cond, NULL)) {
		fprintf(stderr, "Init cond error");
		exit(1);
	}*/

	InitializeCriticalSection(&m->cs);
	InitializeConditionVariable(&m->cond);

	threads[0] = create_thread(_monitor, m);
	threads[1] = create_thread(_timer, m);
	//create_thread(&pid[2], _socket, m);
	skynet_socket_start_socket_thread();

	//struct worker_parm wp[thread];
	struct worker_parm* wp = (struct worker_parm*)skynet_malloc(sizeof(struct worker_parm) * thread);
	for (i=0;i<thread;i++) {
		wp[i].m = m;
		wp[i].id = i;
		threads[2+i] = create_thread(_worker, &wp[i]);
	}

	/*for (i=0;i<thread+3;i++) {
		pthread_join(pid[i], NULL); 
	}*/

	WaitForMultipleObjects(thread+2, threads, TRUE, INFINITE);

	skynet_free(wp);
	skynet_free(threads);
	free_monitor(m);
}

static int
_start_master(const char * master) {
	struct skynet_context *ctx = skynet_context_new("master", master);
	if (ctx == NULL)
		return 1;
	return 0;	
}

void 
skynet_start(struct skynet_config * config) {
	skynet_harbor_init(config->harbor);
	skynet_handle_init(config->harbor);
	skynet_mq_init();
	skynet_module_init(config->module_path);
	skynet_timer_init();
	skynet_socket_init();

	struct skynet_context *ctx;
	ctx = skynet_context_new("logger", config->logger);
	if (ctx == NULL) {
		fprintf(stderr,"launch logger error");
		exit(1);
	}
	
	if (config->standalone) {
		if (_start_master(config->standalone)) {
			fprintf(stderr, "Init fail : mater");
			return;
		}
	}
	// harbor must be init first
	if (skynet_harbor_start(config->master , config->local)) {
		fprintf(stderr, "Init fail : no master");
		return;
	}

	ctx = skynet_context_new("snlua", "launcher");
	if (ctx) {
		skynet_command(ctx, "REG", ".launcher");
		ctx = skynet_context_new("snlua", config->start);
	}

	_start(config->thread);
	//skynet_socket_free();
}

