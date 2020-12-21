/**
* @file core/sync.h
*
* @brief Easier Synchronization primitives
*
* This module defines synchronization primitives for the parallel runtime.
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

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
