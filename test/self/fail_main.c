/**
 * @file test/self/fail_main.c
 *
 * @brief Test: expected failure, main() failure
 *
 * @test Tests that a failure in the test main() function causes a test failure.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	return -1;
}

const struct test_config test_config;
