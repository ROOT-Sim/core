/**
 * @file lib/random/random.c
 *
 * @brief Random Number Generators
 *
 * Piece-Wise Deterministic Random Number Generators.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lib/random/random.h>

#include <core/intrinsics.h>
#include <lib/lib_internal.h>
#include <lib/random/xoroshiro.h>

#include <math.h>
#include <memory.h>

void random_lib_lp_init(void)
{
	uint64_t seed = global_config.prng_seed;
	lp_id_t lid = lp_id_get();
	struct lib_ctx *ctx = lib_ctx_get();
	random_init(ctx->rng_s, lid, seed);
	ctx->unif = NAN;
}

double Random(void)
{
	struct lib_ctx *ctx = lib_ctx_get();
	uint64_t u_val = random_u64(ctx->rng_s);
	if (unlikely(!u_val))
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

uint64_t RandomU64(void)
{
	struct lib_ctx *ctx = lib_ctx_get();
	return random_u64(ctx->rng_s);
}

/**
 * Return a random number according to an Exponential distribution.
 * The mean value of the distribution must be passed as the mean value.
 *
 * @param mean Mean value of the distribution
 * @return A random number
 */
double Expent(double mean)
{
	if (unlikely(mean < 0)) {
		log_log(LOG_WARN, "Passed a negative mean into Expent()");
	}
	return -mean * log(1 - Random());
}

/**
 * Return a random number according to a Standard Normal Distribution
 *
 * @return A random number
 */
double Normal(void)
{
	struct lib_ctx *ctx = lib_ctx_get();
	if (isnan(ctx->unif)) {
		double v1, v2, rsq;
		do {
			v1 = 2.0 * Random() - 1.0;
			v2 = 2.0 * Random() - 1.0;
			rsq = v1 * v1 + v2 * v2;
		} while (rsq >= 1.0 || rsq == 0);

		double fac = sqrt(-2.0 * log(rsq) / rsq);

		// Perform Box-Muller transformation to get two normal deviates.
		// Return one and save the other for next time.
		ctx->unif = v1 * fac;
		return v2 * fac;
	} else {
		// A deviate is already available
		double ret = ctx->unif;
		ctx->unif = NAN;
		return ret;
	}
}

int RandomRange(int min, int max)
{
	return (int)floor(Random() * (max - min + 1)) + min;
}

int RandomRangeNonUniform(int x, int min, int max)
{
	return (((RandomRange(0, x) | RandomRange(min, max))) %
			(max - min + 1)) + min;
}

/**
 * Return a number in according to a Gamma Distribution of Integer Order ia,
 * a waiting time to the ia-th event in a Poisson process of unit mean.
 *
 * @author D. E. Knuth
 * @param ia Integer Order of the Gamma Distribution
 * @return A random number
 */
double Gamma(unsigned ia)
{
	if (unlikely(ia < 1)) {
		log_log(LOG_WARN, "Gamma distribution must have a ia "
				"value >= 1. Defaulting to 1...");
		ia = 1;
	}

	double x;

	if (ia < 6) {
		// Use direct method, adding waiting times
		x = 1.0;
		while (ia--)
			x *= 1 - Random();
		x = -log(x);
	} else {
		double am = ia - 1;
		double v1, v2, e, y, s;
		// Use rejection method
		do {
			do {
				do {
					v1 = Random();
					v2 = 2.0 * Random() - 1.0;
				} while (v1 * v1 + v2 * v2 > 1.0);

				y = v2 / v1;
				s = sqrt(2.0 * am + 1.0);
				x = s * y + am;
			} while (x < 0.0);

			e = (1.0 + y * y) * exp(am * log(x / am) - s * y);
		} while (Random() > e);
	}

	return x;
}

/**
 * Return the waiting time to the next event in a Poisson process of unit mean.
 *
 * @return A random number
 */
double Poisson(void)
{
	return -log(1 - Random());
}

/**
 * Return a random sample from a Zipf distribution.
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
	} while (x > limit || Random() * x * (t - 1.) * b > t * (b - 1.));
	return x;
}
