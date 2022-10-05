/**
 * @file test/framework/rng.c
 *
 * @brief Pseudo random number generator for tests
 *
 * An acceptable-quality pseudo random number generator to be used in tests
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <framework/rng.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define rng_clz(x)                                                                                                     \
	__extension__({                                                                                                \
		assert((x) != 0);                                                                                      \
		__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(x), unsigned), __builtin_clz(x),         \
		    __builtin_choose_expr(__builtin_types_compatible_p(__typeof__(x), unsigned long),                  \
			__builtin_clzl(x),                                                                             \
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(x), unsigned long long),         \
			    __builtin_clzll(x), (void)0)));                                                            \
	})

/**
 * @brief Initializes the random number generator
 * @param rng_state a test_rng_state object which will be initialized
 * @param initseq the seed to use to initialize @a rng_state
 */
void rng_init(test_rng_state *rng_state, test_rng_state initseq)
{
	*rng_state = ((initseq) << 1u) | 1u;
}

/**
 * @brief Computes a pseudo random 64 bit number
 * @param rng_state a test_rng_state object
 * @return a uniformly distributed 64 bit pseudo random number
 */
uint64_t rng_random_u(test_rng_state *rng_state)
{
	const __uint128_t multiplier = (((__uint128_t)0x0fc94e3bf4e9ab32ULL) << 64) + 0x866458cd56f5e605ULL;
	*rng_state *= multiplier;
	return *rng_state >> 64u;
}

/**
 * @brief Computes a pseudo random number in the [0, 1] range
 * @param rng_state a test_rng_state object
 * @return a uniformly distributed pseudo random double value in [0, 1]
 */
double rng_random(test_rng_state *rng_state)
{
	uint64_t u_val = rng_random_u(rng_state);
	double ret = 0.0;
	if(__builtin_expect(u_val != 0, 1)) {
		unsigned lzs = rng_clz(u_val) + 1;
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
uint64_t rng_random_range(test_rng_state *rng_state, uint64_t n)
{
	return (uint64_t)((double)n * rng_random(rng_state));
}

/**
 * @brief Kolmogorov-Smirnov test
 * @param N the number of samples to use
 * @param nBins the number of bins to use
 * @param sample the function to use to generate the samples in [0, 1]
 * @return 0 if the test is passed, 1 otherwise
 */
int rng_ks_test(uint32_t n_samples, double (*sample)(void))
{
	struct ks_bin {
		double min;
		double max;
		uint_fast32_t count;
	};

	struct ks_bin *bins = malloc(sizeof(*bins) * (n_samples + 1));
	if(bins == NULL) {
		perror("Unable to allocate memory in rng_ks_test()");
		return 1;
	}
	memset(bins, 0, sizeof(*bins) * (n_samples + 1));

	int ret = 1;
	for(uint32_t i = 0; i < n_samples; i++) {
		double rf = sample();
		uint32_t k = ceil(rf * n_samples);
		if(k > n_samples)
			goto fail;
		struct ks_bin *b = &bins[k];
		b->min = !b->count || rf < b->min ? rf : b->min;
		b->max = !b->count || rf > b->max ? rf : b->max;
		++b->count;
	}

	double threshold = 1.51743 * sqrt(n_samples);
	uint32_t j = 0;
	for(uint32_t i = 0; i < n_samples + 1; i++) {
		struct ks_bin *b = &bins[i];
		if(b->count == 0)
			continue;

		double z = n_samples * b->min - j;
		j += b->count;
		double k = j - b->max * n_samples;
		z = k > z ? k : z;

		if(z > threshold)
			goto fail;
	}
	ret = 0;

fail:
	free(bins);
	return ret;
}
