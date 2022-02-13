/**
 * @file test/lib/numerical.c
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
#include <math.h>
#include <limits.h>

#include <test.h>
#include <ROOT-Sim.h>
#include <lib/lib.h>
#include <lp/lp.h>


enum distribution {
	RANDOM,
	EXPENT,
	NORMAL,
	GAMMA,
	GAMMA2,
	POISSON,
	ZIPF
};

static double get_sample(enum distribution distr) {

	switch(distr) {
	case RANDOM:
		return Random();

	case EXPENT:
		return Expent(10);

	case NORMAL:
		return Normal();

	case GAMMA:
		return Gamma(2);

	case GAMMA2:
		return Gamma(7);

	case POISSON:
		return Poisson();

	case ZIPF:
		return Zipf(3.0, 10);

	default:
		fprintf(stderr, "Error: unknown distribution\n");
		exit(EXIT_FAILURE);
	}
}

/* Perform the Kolmogorov-Smirnov test on the uniform random number
 * generator.
 */
static int ks_test(uint32_t N, uint32_t nBins, enum distribution distr)
{
	uint32_t i, index, cumulativeSum;
	double rf, ksThreshold, countPerBin;
	uint32_t bins[nBins];

	// Fill the bins
	for (i = 0; i < nBins; i++)
		bins[i] = 0;

	for (i = 0; i < N; i++) {
		rf = get_sample(distr);
		index = floor(rf * nBins);
		if (index >= nBins) // just in case...
			index = nBins - 1;

		bins[index]++;
	}

	// Test the bins
	ksThreshold = 1.358 / sqrt((double)N);
	countPerBin = (double)N / nBins;
	cumulativeSum = 0;
	for (i = 0; i < nBins; i++) {
		cumulativeSum += bins[i];
		if((double) cumulativeSum / N - (i + 1) * countPerBin / N >= ksThreshold)
			return 1;
	}
	return 0;
}

static test_ret_t aux_ks_test(void *d) {
	int passed = 0;
	enum distribution distr = (enum distribution)(long long)d;

	passed += ks_test(1000000, 1000, distr);
	passed += ks_test(100000, 1000, distr);
	passed += ks_test(10000, 100, distr);
	passed += ks_test(1000, 10, distr);
	passed += ks_test(100, 10, distr);

	return passed;
}

static test_ret_t test_random_range_nonuniform(__unused void *_) {
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

static test_ret_t test_random_range(__unused void *_) {
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

	// Mock a fake LP
	struct lp_ctx lp = {0};
	lp.lib_ctx = malloc(sizeof(*current_lp->lib_ctx));
	lp.lib_ctx->rng_s[0] = 7319936632422683443ULL;
	lp.lib_ctx->rng_s[1] = 2268344373199366324ULL;
	lp.lib_ctx->rng_s[2] = 3443862242366399137ULL;
	lp.lib_ctx->rng_s[3] = 2366399137344386224ULL;
	current_lp = &lp;

	test("Kolmogorov-Smirnov test on Random()", aux_ks_test, (void *)RANDOM);
	test("Functional test on RandomRange()", test_random_range, NULL);
	test("Functional test on RandomRangeNonUniform()", test_random_range_nonuniform, NULL);

	free(lp.lib_ctx);

	finish();
}
