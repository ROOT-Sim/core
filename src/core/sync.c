/**
* @file core/sync.c
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
#include <core/sync.h>

#include <core/core.h>

/**
 * @brief Synchronizes threads on a barrier
 * @return true if this thread has been elected as leader, false otherwise
 */
bool sync_thread_barrier(void)
{
	static _Thread_local unsigned phase;
	static atomic_uint cs[2];
	atomic_uint *c = cs + (phase & 1U);

	bool l;
	unsigned r;
	if (phase & 2U) {
		l = atomic_fetch_add_explicit(c, -1, memory_order_release) == 1;
		do {
			r = atomic_load_explicit(c, memory_order_relaxed);
		} while(r);
	} else {
		l = !atomic_fetch_add_explicit(c, 1, memory_order_release);
		rid_t thr_cnt = n_threads;
		do {
			r = atomic_load_explicit(c, memory_order_relaxed);
		} while(r != thr_cnt);
	}

	phase = (phase + 1) & 3U;
	return l;
}
