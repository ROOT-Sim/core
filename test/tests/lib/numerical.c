/**
 * @file test/tests/lib/numerical.c
 *
 * @brief Test: rollbackable RNG
 * @todo test all distributions
 * @todo check rollback capabilities
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
*/
#include <test.h>
#include <framework/rng.h>

#include <lp/lp.h>

#include <limits.h>
#include <stdio.h>

static int aux_ks_test(_unused void *_)
{
	test_assert(rng_ks_test(100000000, Random) == 0);
	test_assert(rng_ks_test(10000000, Random) == 0);
	test_assert(rng_ks_test(1000000, Random) == 0);
	test_assert(rng_ks_test(100000, Random) == 0);
	test_assert(rng_ks_test(10000, Random) == 0);
	test_assert(rng_ks_test(1000, Random) == 0);
	test_assert(rng_ks_test(100, Random) == 0);

	return 0;
}

static int random_range_non_uniform_test(_unused void *_) {
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

static int random_range_test(_unused void *_) {
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
	current_lp = test_lp_mock_get();

	test("Kolmogorov-Smirnov test on Random()", aux_ks_test, NULL);
	test("Functional test on RandomRange()", random_range_test, NULL);
	test("Functional test on RandomRangeNonUniform()", random_range_non_uniform_test, NULL);
}
