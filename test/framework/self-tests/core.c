/**
 * @file test/self-tests/core.c
 *
 * @brief Test: Test core functions of the testing framework
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <test.h>
#include <string.h>

test_ret_t test_pass(__unused void *_)
{
	return 0;
}

test_ret_t test_pass_assert(__unused void *_)
{
	test_assert(1);
	check_passed_asserts();
}

test_ret_t test_fail(__unused void *_)
{
	return -1;
}

test_ret_t test_fail_assert(__unused void *_)
{
	test_assert(0);
	check_passed_asserts();
}

test_ret_t test_explicit_fail(__unused void *_)
{
	fail();
}
