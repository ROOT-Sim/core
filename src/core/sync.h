/**
 * @file core/sync.h
 *
 * @brief Easier Synchronization primitives
 *
 * This module defines synchronization primitives for the parallel runtime.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#endif

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
#define spin_pause() _mm_pause()
#else
#define spin_pause()
#endif

/**
 * @brief Locks a spinlock
 * @param lck_p a pointer to the spinlock_t to lock
 */
#define spin_lock(lck_p)                                                                                               \
	__extension__({                                                                                                \
		while(atomic_flag_test_and_set_explicit((lck_p), memory_order_acquire))                                \
			spin_pause();                                                                                  \
	})

/**
 * @brief Executes the trylock operation on a spinlock
 * @param lck_p a pointer to the spinlock_t to try to lock
 * @return true if the lock was acquired successfully, false otherwise
 */
#define spin_trylock(lck_p) !atomic_flag_test_and_set_explicit((lck_p), memory_order_acquire)

/**
 * @brief Unlocks a spinlock
 * @param lck_p a pointer to the spinlock_t to unlock
 */
#define spin_unlock(lck_p) atomic_flag_clear_explicit((lck_p), memory_order_release)

extern bool sync_thread_barrier(void);
