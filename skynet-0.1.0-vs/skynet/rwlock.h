#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#include <Windows.h>

struct rwlock {
	long write;
	long read;
};

static inline void
rwlock_init(struct rwlock *lock) {
	lock->write = 0;
	lock->read = 0;
}

static inline void
rwlock_rlock(struct rwlock *lock) {
	for (;;) {
		while(lock->write) {
			//__sync_synchronize();
			MemoryBarrier();
		}
		//__sync_add_and_fetch(&lock->read,1);
		InterlockedIncrement(&lock->read);
		if (lock->write) {
			//__sync_sub_and_fetch(&lock->read,1);
			InterlockedDecrement(&lock->read);
		} else {
			break;
		}
	}
}

static inline void
rwlock_wlock(struct rwlock *lock) {
	//while (__sync_lock_test_and_set(&lock->write,1)) {}
	while (InterlockedCompareExchange(&lock->write, 1, 1)) {}
	while(lock->read) {
		//__sync_synchronize();
		MemoryBarrier();
	}
}

static inline void
rwlock_wunlock(struct rwlock *lock) {
	//__sync_lock_release(&lock->write);
	InterlockedExchange(&lock->write, 0);
}

static inline void
rwlock_runlock(struct rwlock *lock) {
	//__sync_sub_and_fetch(&lock->read,1);
	InterlockedDecrement(&lock->read);
}

#endif
