/**
* @file test/self-tests/rng.c
*
* @brief Test: Test for the rng used in tests
*
* A simple test to verify statistical properties of the random number generator used in test programs
*
* SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/

#include "test.h"


test_ret_t test_rng(__unused void *_)
{
	test_assert(ks_test(10000000, 1000, test_random_double) == 0);
	test_assert(ks_test(1000000, 1000, test_random_double) == 0);
	test_assert(ks_test(100000, 1000, test_random_double) == 0);
	test_assert(ks_test(10000, 100, test_random_double) == 0);
	test_assert(ks_test(1000, 10, test_random_double) == 0);
	test_assert(ks_test(100, 10, test_random_double) == 0);

	check_passed_asserts();
}
