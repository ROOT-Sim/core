/**
* @file lib/random/random.c
*
* @brief Random Number Generators
*
* Piece-Wise Deterministic Random Number Generators.
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
* 
* @todo Still missing to reimplement some functions:
*  - RandomRange()
*  - RandomRangeNonUniform()
*  - Gamma()
*  - Poisson()
*  - Zipf()
*/
#include <lib/random/random.h>

#include <core/intrinsics.h>
#include <lib/lib_internal.h>
#include <lib/random/xoroshiro.h>

#include <math.h>
#include <memory.h>

void random_lib_lp_init(void)
{
	lp_id_t lid = lp_id_get();
	struct lib_ctx *ctx = lib_ctx_get();
	random_init(ctx->rng_s, lid);
	ctx->unif = NAN;
}

double Random(void)
{
	struct lib_ctx *ctx = lib_ctx_get();
	uint64_t u_val = random_u64(ctx->rng_s);
	if (unlikely(!u_val))
		return 0.0;

	double ret = 0.0;
	unsigned lzs = SAFE_CLZ(u_val) + 1;
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
* This function returns a random number according to an Exponential distribution.
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
* This function returns a number according to a Standard Normal Distribution
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

