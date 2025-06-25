/**
 * @file test/self/fail_explicit.c
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
	test("Test explicit fail", test_fail_on_not_null, (void *)1);
}
