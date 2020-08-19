#pragma once

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef atomic_flag spinlock_t;

#define spin_init(lck_p) spin_unlock(lck_p)

#define spin_pause() __builtin_ia32_pause()

#define spin_lock(lck_p)						\
__extension__({								\
	while(atomic_flag_test_and_set_explicit((lck_p), 		\
		memory_order_acquire))					\
		spin_pause();						\
})

#define spin_trylock(lck_p)						\
	!atomic_flag_test_and_set_explicit((lck_p), memory_order_acquire)

#define spin_unlock(lck_p)						\
	atomic_flag_clear_explicit((lck_p), memory_order_release)

extern bool sync_thread_barrier(void);
