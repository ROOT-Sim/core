/**
 * @file test/self-tests/fail.c
 *
 * @brief Test: Test core functions of the testing framework
 *
 * This is the test that checks if explicit fail works.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <test.h>
#include <string.h>

extern test_ret_t test_explicit_fail(__unused void *_);

int main(void)
{
	init(0);
	test("Test explicit fail", test_explicit_fail, NULL);
	finish();
}
