/**
 * @file test/test/fail_cmp_short.c
 *
 * @brief Test: expected failure, output too short
 *
 * @test Tests that a shorter output than expected causes a test failure.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	test_printf("a");
	return 0;
}

const struct test_config test_config = {
	.expected_output_size = 2,
	.expected_output = "aa"
};
