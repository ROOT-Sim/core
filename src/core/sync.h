#pragma once

#include <stdint.h>
#include <stdatomic.h>

typedef volatile uint64_t spinlock_t;

#define spin_init(lck_p) __extension__({*lck_p = 0;})

#define spin_lock(lck_p)						\
	__extension__({							\
		while(atomic_exchange_explicit(lck_p, 1U, memory_order_relaxed))\
		{}							\
		atomic_thread_fence(memory_order_acquire);		\
	})

#define spin_trylock(lck_p)						\
	__extension__({							\
		uint64_t __ret = 					\
			atomic_exchange_explicit(lck_p, 1U, memory_order_relaxed);\
		atomic_thread_fence(memory_order_acquire);		\
		!__ret;							\
	})

#define spin_unlock(lck_p)						\
	__extension__({							\
		atomic_thread_fence(memory_order_release);		\
		atomic_store_explicit(lck_p, 0U, memory_order_relaxed);	\
	})

