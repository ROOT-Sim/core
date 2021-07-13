/**
 * @file test/self/fail_body.c
 *
 * @brief Test: expected failure, @ref test_config.test_fnc failure
 *
 * @test Tests that a failure in one of the concurrently executed
 * @ref test_config.test_fnc calls causes a test failure.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <stdatomic.h>

/// default number of threads for this test
#define THREADS_CNT 4

/**
 * @brief Test body
 * @return 0 if successful, -1 otherwise
 */
static int fail_test(void)
{
	static atomic_uint cnt = THREADS_CNT;
	if (atomic_fetch_sub_explicit(&cnt, 1U, memory_order_relaxed) == 1)
		return -1;
	return 0;
}

const struct test_config test_config = {
	.threads_count = THREADS_CNT,
	.test_fnc = fail_test
};
