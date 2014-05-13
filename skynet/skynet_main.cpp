/*#include "skynet_timer.h"
#include "rwlock.h"

#include <windows.h>
#include <process.h>


static unsigned __stdcall _timer(void* p)
{
	struct monitor * m = (struct monitor*)p;
	for (;;) {
		skynet_updatetime();
		CHECK_ABORT
		//	wakeup(m,m->count-1);

		SleepEx(2, true);
		//usleep(2500);
	}
	// wakeup socket thread
	//skynet_socket_exit();
	// wakeup all worker thread
	//pthread_cond_broadcast(&m->cond);
}

int main(void)
{
	skynet_timer_init();

	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, _timer, NULL, 0, NULL);

	skynet_timeout(10, 100, 10);
	skynet_timeout(20, 300, 10);
	skynet_timeout(30, 500, 10);
	WaitForSingleObject(hThread, INFINITE);
	return 0;
}
*/

