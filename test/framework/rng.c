/**
 * @file test/tests/integration/model/test_rng.h
 *
 * @brief Pseudo random number generator for tests
 *
 * An acceptable-quality pseudo random number generator to be used in tests
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <string.h>
#include <math.h>

#include "core/intrinsics.h"

static _Thread_local test_rng_state seed;

/// The multiplier of this linear congruential PRNG generator
#define LCG_MULTIPLIER ((((__uint128_t)0x0fc94e3bf4e9ab32ULL) << 64) + 0x866458cd56f5e605ULL)

/**
 * @brief Initializes the random number generator
 * @param rng_state a test_rng_state object which will be initialized
 * @param initseq the seed to use to initialize @a rng_state
 */
 void lcg_init(test_rng_state *rng_state, test_rng_state initseq)
{
	*rng_state = ((initseq) << 1u) | 1u;
}

/**
 * @brief Computes a pseudo random 64 bit number
 * @param rng_state a test_rng_state object
 * @return a uniformly distributed 64 bit pseudo random number
 */
__uint64_t lcg_random_u(test_rng_state *rng_state)
{
	*rng_state *= LCG_MULTIPLIER;
	return *rng_state >> 64u;
}

/**
 * @brief Computes a pseudo random number in the [0, 1] range
 * @param rng_state a test_rng_state object
 * @return a uniformly distributed pseudo random double value in [0, 1]
 */
double lcg_random(test_rng_state *rng_state)
{
	__uint64_t u_val = lcg_random_u(rng_state);
	double ret = 0.0;
	if(__builtin_expect(!!u_val, 1)) {
		unsigned lzs = intrinsics_clz(u_val) + 1;
		u_val <<= lzs;
		u_val >>= 12;

		__uint64_t _exp = 1023 - lzs;
		u_val |= _exp << 52;

		memcpy(&ret, &u_val, sizeof(double));
	}
	return ret;
}

/**
 * @brief Computes a pseudo random number in the [0, n] range
 * @param rng_state a test_rng_state object
 * @param n the upper bound of the [0, n] range
 * @return a uniformly distributed pseudo random double value in [0, n]
 */
__uint64_t lcg_random_range(test_rng_state *rng_state, __uint64_t n)
{
	return lcg_random_u(rng_state) % n;
}

/**
 * @brief Computes a pseudo random number in the [0, 1] range
 * @param rng_state a test_rng_state object
 * @return a uniformly distributed pseudo random double value in [0, 1]
 *
 * This is the per-thread version of lcg_random()
 */
double test_random_double() {
	return lcg_random(&seed);
}

/**
 * @brief Computes a pseudo random number in the [0, n] range
 * @param n
 * @return a uniformly distributed pseudo random double value in [0, n]
 *
 * This is the per-thread version of lcg_random_range()
 */
__uint64_t test_random_range(__uint64_t n) {
	return lcg_random_range(&seed, n);
}

/**
 * @brief Computes a pseudo random 64 bit number
 * @return a uniformly distributed 64 bit pseudo random number
 *
 * This is the per-thread version of lcg_random_u()
 */
__uint64_t test_random_u(void)
{
	return lcg_random_u(&seed);
}

/**
 * @brief Initialize per-thread random number generator
 */
void test_random_init(void)
{
	lcg_init(&seed, 0x1234567890abcdefULL);
}



/**
 * @brief Kolmogorov-Smirnov test
 * @param N the number of samples to use
 * @param nBins the number of bins to use
 * @param sample the function to use to generate the samples in [0, 1]
 * @return 0 if the test is passed, 1 otherwise
 */
int ks_test(__uint32_t N, __uint32_t nBins, double (*sample)(void))
{
	__uint32_t i, index, cumulativeSum;
	double rf, ksThreshold, countPerBin;
	__uint32_t bins[nBins];

	// Fill the bins
	for (i = 0; i < nBins; i++)
		bins[i] = 0;

	for (i = 0; i < N; i++) {
		rf = sample();
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
