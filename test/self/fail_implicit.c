/**
 * @file test/self/fail_implicit.c
 *
 * @brief Test: Test core functions of the testing framework
 *
 * This is the src that checks if explicit fail works.
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include "stubs.h"

int main(void)
{
	test("Test implicit fail", test_want_arg_null, (void *)1);
}
