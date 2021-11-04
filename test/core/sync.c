/**
 * @file test/core/synchronize.c
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


#define N_THREADS 16
#define REPS_COUNT 100000

static atomic_uint counter;
static spinlock_t spin;
static mrswlock_t rw_lock;
static unsigned k = 0;
static unsigned d = 0;

_Static_assert(REPS_COUNT % 5 == 0, "Spinlock test is not correct");

static thr_ret_t THREAD_CALL_CONV sync_test(void *null)
{
	(void)null;
	unsigned long long ret = 0;
	unsigned j = REPS_COUNT;
	while (j--) {
		atomic_fetch_add_explicit(&counter, 1U, memory_order_relaxed);
		sync_thread_barrier();
		unsigned val = atomic_load_explicit(&counter, memory_order_relaxed);
		sync_thread_barrier();
		ret -= (int)(val % N_THREADS);
	}

	sync_thread_barrier();

	j = REPS_COUNT;
	while (j--) {
		spin_lock(&spin);
		switch (k % 5) {
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
			__builtin_unreachable();
		}
		spin_unlock(&spin);
	}

	sync_thread_barrier();
	ret -= k != ((REPS_COUNT * 5 * N_THREADS + 2) / 3);
	sync_thread_barrier();

	j = REPS_COUNT;
	while (j--) {
		mrswlock_wlock(&rw_lock, (int)N_THREADS);
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
		mrswlock_wunlock(&rw_lock, N_THREADS);
	}

	sync_thread_barrier();
	ret -= d != k;
	sync_thread_barrier();
	d = 0;
	k = 0;
	sync_thread_barrier();

	j = REPS_COUNT;
	if (rid) {
		while (j--) {
			mrswlock_rlock(&rw_lock);
			ret -= d != k;
			mrswlock_runlock(&rw_lock);
		}
	} else {
		while (j--) {
			mrswlock_wlock(&rw_lock, N_THREADS);
			d += 3;
			k += 2;
			k += d % 5;
			d += 2;
			mrswlock_wunlock(&rw_lock, N_THREADS);
		}
	}

	sync_thread_barrier();
	return (thr_ret_t)ret;
}

void foo() {}

struct simulation_configuration conf = {
    .lps = 16,
    .n_threads = N_THREADS,
    .dispatcher = (ProcessEvent_t)foo,
    .committed = (CanEnd_t)foo,
};


int main(void)
{
	init();

	RootsimInit(&conf);
	mrswlock_init(&rw_lock, N_THREADS);

	parallel_test("Testing synchronization barrier", N_THREADS, sync_test);

	finish();
}