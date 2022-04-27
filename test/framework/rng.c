/**
 * @file test/framework/rng.c
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
uint64_t lcg_random_u(test_rng_state *rng_state)
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
	uint64_t u_val = lcg_random_u(rng_state);
	double ret = 0.0;
	if(__builtin_expect(u_val != 0, 1)) {
		unsigned lzs = intrinsics_clz(u_val) + 1;
		u_val <<= lzs;
		u_val >>= 12;

		uint64_t _exp = 1023 - lzs;
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
uint64_t lcg_random_range(test_rng_state *rng_state, uint64_t n)
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
uint64_t test_random_range(uint64_t n) {
	return lcg_random_range(&seed, n);
}

/**
 * @brief Computes a pseudo random 64 bit number
 * @return a uniformly distributed 64 bit pseudo random number
 *
 * This is the per-thread version of lcg_random_u()
 */
uint64_t test_random_u(void)
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
int ks_test(uint32_t n_samples, uint32_t n_bins, double (*sample)(void))
{
	uint32_t bins[n_bins];
	memset(bins, 0, sizeof(bins));

	for (uint32_t i = 0; i < n_samples; i++) {
		double rf = sample();
		uint32_t k = floor(rf * n_bins);
		if (k >= n_bins) // just in case...
			k = n_bins - 1;

		bins[k]++;
	}

	// Test the bins
	double threshold = 1.358 / sqrt((double)n_samples);
	double count_per_bin = (double)n_samples / n_bins;
	uint32_t sum = 0;
	for (uint32_t i = 0; i < n_bins; i++) {
		sum += bins[i];
		if((double)sum / n_samples - (i + 1) * count_per_bin / n_samples >= threshold)
			return 1;
	}
	return 0;
}
