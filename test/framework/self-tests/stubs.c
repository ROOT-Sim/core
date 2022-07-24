/**
 * @file test/self-tests/core.c
 *
 * @brief Test: Test core functions of the testing framework
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <stdatomic.h>
#include <string.h>

int test_want_arg_null(void *arg)
{
	return arg != NULL;
}

int test_assert_arg_null(void *arg)
{
	test_assert(arg == NULL);
	return 0;
}

int test_fail_on_not_null(void *arg)
{
	if(arg != NULL)
		test_fail();
	return 0;
}

int test_fail_once(_unused void *arg)
{
	static atomic_flag failed = ATOMIC_FLAG_INIT;
	return !atomic_flag_test_and_set_explicit(&failed, memory_order_relaxed);
}
