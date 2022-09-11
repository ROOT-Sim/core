/**
 * @file test/framework/self-tests/main.c
 *
 * @brief Test: Main program of the self-tests tests for the testing framework
 *
 * Entry point for the test cases of the testing framework
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <framework/self-tests/stubs.h>
#include <framework/rng.h>
#include <framework/thread.h>

#include <math.h>
#include <stdatomic.h>
#include <stdio.h>

#define REP_COUNT 25
#define THREAD_COUNT_ID_TEST 100

static _Bool presence[THREAD_COUNT_ID_TEST] = {0};
static _Atomic unsigned count = 0;

static int thread_fnc(_unused void *_)
{
	atomic_fetch_add_explicit(&count, 1U, memory_order_relaxed);
	return 0;
}

static int thread_execution(_unused void *_)
{
	char desc[128] = "Testing first thread execution phase";

	test_parallel(desc, thread_fnc, NULL, 0);
	test_assert(count == test_thread_cores_count());

	for(unsigned i = 1; i < REP_COUNT; i++) {
		snprintf(desc, sizeof(desc), "Testing thread execution phase %u", i);
		atomic_store_explicit(&count, 0U, memory_order_relaxed);
		test_parallel(desc, thread_fnc, NULL, i);
		test_assert(count == i);
	}
	return 0;
}

static int test_rng(_unused void *_)
{
	test_assert(rng_ks_test(10000000, test_random_double) == 0);
	test_assert(rng_ks_test(1000000, test_random_double) == 0);
	test_assert(rng_ks_test(100000, test_random_double) == 0);
	test_assert(rng_ks_test(10000, test_random_double) == 0);
	test_assert(rng_ks_test(1000, test_random_double) == 0);
	test_assert(rng_ks_test(100, test_random_double) == 0);
	return 0;
}

static double test_exponential_prng(void)
{
	double ret = log(1 - test_random_double());
	ret = ret > 1 ? 1 : ret;
	return ret;
}

static int test_fail_rng(_unused void *_)
{
	test_assert(rng_ks_test(1000, test_exponential_prng) == 0);
	return 0;
}

static int test_single_thread_id(_unused void *_)
{
	return test_parallel_thread_id() != 0;
}

static int test_multi_thread_id(_unused void *_)
{
	if(presence[test_parallel_thread_id()])
		return -1;

	presence[test_parallel_thread_id()] = 1;
	return 0;
}

static int test_sleep(_unused void *_)
{
	static atomic_flag flag = ATOMIC_FLAG_INIT;
	if(test_parallel_thread_id()) {
		test_thread_sleep(500);
		return !atomic_flag_test_and_set(&flag);
	}
	return atomic_flag_test_and_set(&flag);
}

int main(void)
{
	test("Test passing simple test", test_want_arg_null, NULL);
	test("Test passing assert test", test_assert_arg_null, NULL);
	test("Test passing fail test", test_fail_on_not_null, NULL);

	test_xf("Test failing simple test", test_want_arg_null, (void *)1);
	test_xf("Test failing assert test", test_assert_arg_null, (void *)1);
	test_xf("Test failing fail test", test_fail_on_not_null, (void *)1);

	test_parallel("Test pseudo multithread passing simple test", test_want_arg_null, NULL, 1);
	test_parallel("Test pseudo multithread passing assert test", test_assert_arg_null, NULL, 1);
	test_parallel("Test pseudo multithread passing fail test", test_fail_on_not_null, NULL, 1);

	test_parallel("Test multithread passing simple test", test_want_arg_null, NULL, 0);
	test_parallel("Test multithread passing assert test", test_assert_arg_null, NULL, 0);
	test_parallel("Test multithread passing fail test", test_fail_on_not_null, NULL, 0);

	test_parallel("Test random number generator", test_rng, NULL, 12);
	test_xf("Test random number generator fail test", test_fail_rng, NULL);

	test_parallel("Test pseudo multithread thread id", test_single_thread_id, NULL, 1);
	test_parallel("Test multithread thread id", test_multi_thread_id, NULL, THREAD_COUNT_ID_TEST);

	for(unsigned i = 0; i < THREAD_COUNT_ID_TEST; ++i)
		test_assert(presence[i]);

	test("Test threaded execution", thread_execution, NULL);
	test("Test thread sleep", test_sleep, NULL);

	test_assert(1);
}
