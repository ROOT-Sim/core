/**
 * @file core/sync.h
 *
 * @brief Easier Synchronization primitives
 *
 * This module defines synchronization primitives for the parallel runtime.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

/// The type of a spinlock, an efficient lock primitive in contended scenarios
typedef atomic_flag spinlock_t;

/**
 * @brief Initializes a spinlock
 * @param lck_p a pointer to the spinlock_t to initialize
 */
#define spin_init(lck_p) spin_unlock(lck_p)

/**
 * @def spin_pause()
 * @brief Tells the compiler that we are inside a spin loop
 */
#if defined(__x86_64__) || defined(__i386__)
#define spin_pause() __builtin_ia32_pause()
#else
#define spin_pause()
#endif

/**
 * @brief Locks a spinlock
 * @param lck_p a pointer to the spinlock_t to lock
 */
#define spin_lock(lck_p)						\
__extension__({								\
	while(atomic_flag_test_and_set_explicit((lck_p), 		\
		memory_order_acquire))					\
		spin_pause();						\
})

/**
 * @brief Executes the trylock operation on a spinlock
 * @param lck_p a pointer to the spinlock_t to try to lock
 * @return true if the lock was acquired successfully, false otherwise
 */
#define spin_trylock(lck_p)						\
	!atomic_flag_test_and_set_explicit((lck_p), memory_order_acquire)

/**
 * @brief Unlocks a spinlock
 * @param lck_p a pointer to the spinlock_t to unlock
 */
#define spin_unlock(lck_p)						\
	atomic_flag_clear_explicit((lck_p), memory_order_release)


typedef atomic_int mrswlock_t;

#define mrswlock_init(lck_p, r_cnt) atomic_store_explicit(lck_p, r_cnt, memory_order_relaxed)

#define mrswlock_rlock(lck_p)						\
__extension__({								\
	do {								\
		int i = atomic_fetch_add_explicit((lck_p), -1, 		\
					memory_order_relaxed);		\
		if (likely(i > 0))					\
			break;						\
		atomic_fetch_add_explicit((lck_p), 1, 			\
			memory_order_relaxed);				\
		do {							\
			spin_pause();					\
		} while (atomic_load_explicit((lck_p), 			\
				memory_order_relaxed) <= 0);		\
	} while (1);							\
	atomic_thread_fence(memory_order_acquire);			\
})

#define mrswlock_wlock(lck_p, r_cnt)					\
__extension__({								\
	do {								\
		int i = atomic_fetch_add_explicit((lck_p), -(r_cnt), 	\
				memory_order_relaxed);			\
		if (likely(i == (int)(r_cnt))) 				\
			break;						\
		if (likely(i > 0)) {					\
			do {						\
				spin_pause();				\
			} while (atomic_load_explicit((lck_p), 		\
					memory_order_relaxed) < 0);	\
			break;						\
		}							\
		atomic_fetch_add_explicit((lck_p), (r_cnt),		\
			memory_order_relaxed);				\
		do {							\
			spin_pause();					\
		} while (atomic_load_explicit((lck_p), 			\
				memory_order_relaxed) < 0);		\
	} while(1);							\
	atomic_thread_fence(memory_order_acquire);			\
})

#define mrswlock_runlock(lck_p)						\
__extension__({								\
	atomic_thread_fence(memory_order_release);			\
	atomic_fetch_add_explicit((lck_p), 1, memory_order_relaxed);	\
})

#define mrswlock_wunlock(lck_p, r_cnt)					\
__extension__({								\
	atomic_thread_fence(memory_order_release);			\
	atomic_fetch_add_explicit((lck_p), (r_cnt), 			\
		memory_order_relaxed);					\
})

extern bool sync_thread_barrier(void);
