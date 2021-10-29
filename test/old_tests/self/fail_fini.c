/**
 * @file test/self/fail_fini.c
 *
 * @brief Test: expected failure, @ref test_config.test_fini_fnc failure
 *
 * @test Tests that a failure in the test finalization function
 * @ref test_config.test_fini_fnc call causes a test failure.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

/**
 * @brief Test body
 * @return -1
 */
static int fail_test(void)
{
	return -1;
}

const struct test_config test_config = {
	.test_fini_fnc = fail_test
};
