/**
 * @file lib/random/random.c
 *
 * @brief Random Number Generators
 *
 * Piece-Wise Deterministic Random Number Generators.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lib/random/random.h>

#include <core/core.h>
#include <core/intrinsics.h>
#include <lib/random/xoroshiro.h>
#include <lib/random/xxtea.h>
#include <lp/lp.h>

#include <math.h>

static const uint32_t xxtea_seeding_key[4] = {UINT32_C(0xd0a8f58a), UINT32_C(0x33359424), UINT32_C(0x09baa55b),
    UINT32_C(0x80e1bdb0)};

/**
 * @brief Initialize the rollbackable RNG library of the current LP
 */
void random_lib_lp_init(void)
{
	uint64_t seed = global_config.prng_seed;
	uint64_t lid = current_lp - lps;
	struct rng_ctx *ctx = current_lp->rng_ctx;
	ctx->state[0] = lid;
	ctx->state[1] = seed;
	ctx->state[2] = lid;
	ctx->state[3] = seed;
	xxtea_encode((uint32_t *)ctx->state, 8, xxtea_seeding_key);
}

/**
 * @brief Return a random 64-bit value
 * @return The random number
 */
uint64_t RandomU64(void)
{
	struct rng_ctx *ctx = current_lp->rng_ctx;
	return random_u64(ctx->state);
}

/**
 * @brief Return a random value in [0,1] according to a uniform distribution
 * @return The random number
 */
double Random(void)
{
	uint64_t u_val = RandomU64();
	if(unlikely(!u_val))
		return 0.0;

	double ret = 0.0;
	unsigned lzs = intrinsics_clz(u_val) + 1;
	u_val <<= lzs;
	u_val >>= 12;

	uint64_t exp = 1023 - lzs;
	u_val |= exp << 52;

	memcpy(&ret, &u_val, sizeof(double));
	return ret;
}

/**
 * @brief Return a pair of independent random numbers according to a Standard Normal Distribution
 * @return A pair of random numbers
 */
double Normal(void)
{
	double v1, v2, rsq;
	do {
		v1 = 2.0 * Random() - 1.0;
		v2 = 2.0 * Random() - 1.0;
		rsq = v1 * v1 + v2 * v2;
	} while(rsq >= 1.0 || rsq == 0);

	double fac = sqrt(-2.0 * log(rsq) / rsq);
	return v1 * fac; // also v2 * fac is normally distributed and independent
}

int RandomRange(int min, int max)
{
	return (int)floor(Random() * (max - min + 1)) + min;
}

int RandomRangeNonUniform(int x, int min, int max)
{
	return (((RandomRange(0, x) | RandomRange(min, max))) % (max - min + 1)) + min;
}

/**
 * @brief Return a number in according to a Gamma Distribution of Integer Order ia
 * Corresponds to the waiting time to the ia-th event in a Poisson process of unit mean.
 *
 * @author D. E. Knuth
 * @param ia Integer Order of the Gamma Distribution
 * @return A random number
 */
double Gamma(unsigned ia)
{
	if(ia < 6) {
		// Use direct method, adding waiting times
		double x = 1.0;
		while(ia--)
			x *= 1 - Random();
		return -log(x);
	}

	double x, y, s;
	double am = ia - 1;
	// Use rejection method
	do {
		double v1, v2;
		do {
			v1 = Random();
			v2 = 2.0 * Random() - 1.0;
		} while(v1 * v1 + v2 * v2 > 1.0);

		y = v2 / v1;
		s = sqrt(2.0 * am + 1.0) * y;
		x = s + am;
	} while(x < 0.0 || Random() > (1.0 + y * y) * exp(am * log(x / am) - s));

	return x;
}

/**
 * @brief Return a random number according to an Exponential distribution with unit mean
 * Corresponds to the waiting time to the next event in a Poisson process of unit mean.
 *
 * @return A random number
 */
double Poisson(void)
{
	return -log(1 - Random());
}

/**
 * @brief Return a random sample from a Zipf distribution
 * Based on the rejection method by Luc Devroye for sampling:
 * "Non-Uniform Random Variate Generation, page 550, Springer-Verlag, 1986
 *
 * @param skew The skew of the distribution
 * @param limit The largest sample to retrieve
 * @return A random number
 */
unsigned Zipf(double skew, unsigned limit)
{
	double b = pow(2., skew - 1.);
	double x, t;
	do {
		x = floor(pow(Random(), -1. / skew - 1.));
		t = pow(1. + 1. / x, skew - 1.);
	} while(x > limit || Random() * x * (t - 1.) * b > t * (b - 1.));
	return (unsigned)x;
}
