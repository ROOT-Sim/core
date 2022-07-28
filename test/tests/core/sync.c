/**
 * @file test/tests/core/sync.c
 *
 * @brief Test: synchronization primitives test
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <framework/thread.h>

#include <core/core.h>
#include <core/sync.h>

#include <stdatomic.h>

#define THREAD_REP 10000

static unsigned n_threads;
static atomic_uint counter;
static spinlock_t spin;
static mrswlock_t rw_lock;
static unsigned k = 0;
static unsigned d = 0;

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

static int mrswlock_test(_unused void *_)
{
	int ret = 0;
	unsigned j = THREAD_REP;

	while(j--) {
		mrswlock_wlock(&rw_lock, (int)n_threads);
		switch(d % 5) {
			case 0:
			case 2:
				d += 1;
				__attribute__((fallthrough));
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

	sync_thread_barrier();
	ret += d != ((THREAD_REP * 5 * n_threads + 2) / 3);
	sync_thread_barrier();
	d = 0;
	k = 0;
	sync_thread_barrier();

	j = THREAD_REP;
	if(rid) {
		while(j--) {
			mrswlock_rlock(&rw_lock);
			ret += d != k;
			mrswlock_runlock(&rw_lock);
		}
	} else {
		while(j--) {
			mrswlock_wlock(&rw_lock, n_threads);
			d += 3;
			k += 2;
			k += d % 5;
			d += 2;
			mrswlock_wunlock(&rw_lock, n_threads);
		}
	}

	sync_thread_barrier();
	return ret;
}

int main(void)
{
	n_threads = test_thread_cores_count();
	global_config.n_threads = n_threads;
	test_parallel("Testing synchronization barrier", sync_barrier_test, NULL, n_threads);
	test_parallel("Testing spinlock", spinlock_test, NULL, n_threads);
	mrswlock_init(&rw_lock, n_threads);
	test_parallel("Testing mrswlock", mrswlock_test, NULL, n_threads);
}
