/**
 * @file core/sync.c
 *
 * @brief Easier Synchronization primitives
 *
 * This module defines synchronization primitives for the parallel runtime.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
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
		l = atomic_fetch_add_explicit(c, -1, memory_order_acq_rel) == 1;
		do {
			r = atomic_load_explicit(c, memory_order_relaxed);
		} while (r);
	} else {
		l = !atomic_fetch_add_explicit(c, 1, memory_order_acq_rel);
		rid_t thr_cnt = n_threads;
		do {
			r = atomic_load_explicit(c, memory_order_relaxed);
		} while (r != thr_cnt);
	}

	phase = (phase + 1) & 3U;
	return l;
}
