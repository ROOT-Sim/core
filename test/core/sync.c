/**
 * @file test/tests/core/sync.c
 *
 * @brief Test: synchronization primitives test
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <core/core.h>
#include <core/sync.h>

#include <stdatomic.h>

#define THREAD_REP 10000

static unsigned n_threads;
static atomic_uint counter;
static spinlock_t spin;
static unsigned k = 0;

_Static_assert(THREAD_REP % 5 == 0, "THREAD_REP must be a multiple of 5 for the spinlock test to work.");

static int sync_barrier_test(_unused void *_)
{
	int ret = 0;
	unsigned j = THREAD_REP;
	while(j--) {
		atomic_fetch_add_explicit(&counter, 1U, memory_order_relaxed);
		sync_thread_barrier();
		unsigned val = atomic_load_explicit(&counter, memory_order_relaxed);
		sync_thread_barrier();
		ret += (int)(val % n_threads);
	}

	return ret;
}

static int spinlock_test(_unused void *_)
{
	int ret = 0;
	unsigned j = THREAD_REP;

	while(j--) {
		spin_lock(&spin);
		switch(k % 5) {
			case 0:
			case 2:
				k += 1;
				__attribute__((fallthrough));
			case 4:
				k += 1;
				break;
			case 3:
			case 1:
				k += 5;
				break;
			default:
				test_fail();
		}
		spin_unlock(&spin);
	}

	sync_thread_barrier();
	ret += k != ((THREAD_REP * 5 * n_threads + 2) / 3);
	// FIXME: these 3 barriers are necessary to reset the internal barriers counts before the next test
	sync_thread_barrier();
	sync_thread_barrier();
	sync_thread_barrier();

	return ret;
}

int main(void)
{
	n_threads = test_thread_cores_count();
	global_config.n_threads = n_threads;
	test_parallel("Testing synchronization barrier", sync_barrier_test, NULL, n_threads);
	test_parallel("Testing spinlock", spinlock_test, NULL, n_threads);
}
