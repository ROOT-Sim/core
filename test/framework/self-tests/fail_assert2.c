/**
 * @file test/self-tests/fail_assert2.c
 *
 * @brief Test: Test core functions of the testing framework
 *
 * This is the test that checks if assert can fail a test.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <test.h>
#include <string.h>

extern test_ret_t test_pass(__unused void *_);

int main(void)
{
	init(0);
	test_xf("Test unexpected pass", test_pass, NULL);
	finish();
}
