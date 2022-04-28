/**
 * @file test/tests/lib/numerical.c
 *
 * @brief Test: rollbackable RNG
 * @todo test all distributions
 * @todo check rollback capabilities
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
*/
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <test.h>
#include <ROOT-Sim.h>

#include "lib/lib.h"
#include "lp/lp.h"

static test_ret_t aux_ks_test(__unused void *_) {
	test_assert(ks_test(100000000, 1000, Random) == 0);
	test_assert(ks_test(1000000, 1000, Random) == 0);
	test_assert(ks_test(100000, 1000, Random) == 0);
	test_assert(ks_test(10000, 100, Random) == 0);
	test_assert(ks_test(1000, 10, Random) == 0);
	test_assert(ks_test(100, 10, Random) == 0);

	check_passed_asserts();
}

static test_ret_t random_range_non_uniform_test(__unused void *_) {
	int passed = 0;
	int x, min, max, r, i;

	for(i = 0; i < 1000000; i++) {
		x = INT_MAX * Random();
		max = INT_MAX * Random();
		min = (int)(max * Random());
		r = RandomRangeNonUniform(x, min, max);

		if(r < min || r > max)
			passed = 1;
	}

	return passed;
}

static test_ret_t random_range_test(__unused void *_) {
	int passed = 0;
	int min, max, r, i;

	for(i = 0; i < 1000000; i++) {
		max = INT_MAX * Random();
		min = (int)(max * Random());
		r = RandomRange(min, max);

		if(r < min || r > max)
			passed = 1;
	}

	return passed;
}


int main(void)
{
	init(0);

	current_lp = mock_lp();

	test("Kolmogorov-Smirnov test on Random()", aux_ks_test, NULL);
	test("Functional test on RandomRange()", random_range_test, NULL);
	test("Functional test on RandomRangeNonUniform()", random_range_non_uniform_test, NULL);

	finish();
}
