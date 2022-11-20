/**
 * @file test/framework/self-tests/unexp_pass_assert.c
 *
 * @brief Test: Test core functions of the testing framework
 *
 * This is the test that checks if assert can fail a test.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <framework/self-tests/stubs.h>
#include <stdlib.h>

int main(void)
{
	test_xf("Test unexpected pass with asserts", test_assert_arg_null, NULL);
}
