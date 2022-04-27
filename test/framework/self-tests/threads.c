/**
* @file test/self-tests/threads.c
*
* @brief Test: Thread pool test
*
* Tests to test the thread pool implementation used for parallel tests
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#define REP_COUNT 25

_Atomic unsigned count = 0;

static test_ret_t thread(__unused void *_)
{
	count++;
	return 0;
}

#define DESC_LEN 128
test_ret_t thread_execution(__unused void *_)
{
	char desc[DESC_LEN];

	for(int i = 1; i < REP_COUNT+1; i++) {
		snprintf(desc, DESC_LEN, "Testing thread execution phase %d", i);
		parallel_test(desc, thread, NULL);
		test_assert(count == test_thread_pool_size() * i);
	}
	check_passed_asserts();
}
