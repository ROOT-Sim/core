/**
 * @file test/core/sync_test.c
 *
 * @brief Test: synchronization primitives test
 * @todo test the spinlock as well
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <core/core.h>
#include <core/sync.h>

#include <stdatomic.h>

#define REPS_COUNT 100000

static atomic_uint counter;
static spinlock_t spin;
static mrswlock_t rw_lock;
static unsigned k = 0;
static unsigned d = 0;

_Static_assert(REPS_COUNT % 5 == 0, "Spinlock test is not correct");

static int sync_init(void)
{
	mrswlock_init(&rw_lock, n_threads);
	return 0;
}

static int sync_test_init(void)
{
	n_threads = THREAD_CNT;
	return 0;
}

static int sync_test(void)
{
	int ret = 0;
	unsigned i = REPS_COUNT;
	while (i--) {
		atomic_fetch_add_explicit(&counter, 1U, memory_order_relaxed);
		sync_thread_barrier();
		unsigned val = atomic_load_explicit(&counter, memory_order_relaxed);
		sync_thread_barrier();
		ret -= val % n_threads;
	}

	test_thread_barrier();

	i = REPS_COUNT;
	while (i--) {
		spin_lock(&spin);
		switch (k % 5) {
		case 0:
		case 2:
			k += 1;
			/* fallthrough */
		case 4:
			k += 1;
			break;
		case 3:
		case 1:
			k += 5;
			break;
		default:
			__builtin_unreachable();
		}
		spin_unlock(&spin);
	}

	test_thread_barrier();
	ret -= k != ((REPS_COUNT * 5 * n_threads + 2) / 3);
	test_thread_barrier();

	i = REPS_COUNT;
	while (i--) {
		mrswlock_wlock(&rw_lock, (int)n_threads);
		switch (d % 5) {
		case 0:
		case 2:
			d += 1;
			/* fallthrough */
		case 4:
			d += 1;
			break;
		case 3:
		case 1:
			d += 5;
			break;
		default:
			__builtin_unreachable();
		}
		mrswlock_wunlock(&rw_lock, n_threads);
	}

	test_thread_barrier();
	ret -= d != k;
	test_thread_barrier();
	d = 0;
	k = 0;
	test_thread_barrier();

	i = REPS_COUNT;
	if (rid) {
		while (i--) {
			mrswlock_rlock(&rw_lock);
			ret -= d != k;
			mrswlock_runlock(&rw_lock);
		}
	} else {
		while (i--) {
			mrswlock_wlock(&rw_lock, n_threads);
			d += 3;
			k += 2;
			k += d % 5;
			d += 2;
			mrswlock_wunlock(&rw_lock, n_threads);
		}
	}

	test_thread_barrier();
	return ret;
}

const struct test_config test_config = {
	.threads_count = THREAD_CNT,
	.test_init_fnc = sync_init,
	.test_fnc = sync_test,
};
