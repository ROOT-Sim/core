#pragma once

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef atomic_uint_fast8_t spinlock_t;

#define spin_init(lck_p) do{ *(lck_p) = 0; }while(0)

#define spin_lock(lck_p)						\
	__extension__({							\
		while(							\
			atomic_exchange_explicit(			\
				(lck_p),				\
				1U,					\
				memory_order_relaxed			\
			)						\
		){}							\
		atomic_thread_fence(memory_order_acquire);		\
	})

#define spin_trylock(lck_p)						\
	(!atomic_exchange_explicit((lck_p), 1U, memory_order_acquire))

#define spin_unlock(lck_p)						\
	atomic_store_explicit((lck_p), 0U, memory_order_release)

extern bool sync_thread_barrier(void);
