/**
 * @file test/test_rng.h
 *
 * @brief Pseudo random number generator for tests
 *
 * An acceptable quality pseudo random number generator to be used in tests
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
 */
#pragma once

#include <core/intrinsics.h>

#include <stdint.h>

/// The type of this pseudo random number generator state
typedef __uint128_t test_rng_t;

/// The multiplier of this linear congruential PRNG generator
#define LCG_MULTIPLIER \
	((((__uint128_t)0x0fc94e3bf4e9ab32ULL) << 64) + 0x866458cd56f5e605ULL)

/**
 * @brief Initializes the random number generator
 * @param rng_state a test_rng_t object which will be initialized
 * @param initseq the seed to use to initialize @a rng_state
 */
#define lcg_init(rng_state, initseq) (rng_state) = ((initseq) << 1u) | 1u

/**
 * @brief Computes a pseudo random 64 bit number
 * @param rng_state a test_rng_t object
 * @return a uniformly distributed 64 bit pseudo random number
 */
#define lcg_random_u(rng_state)						\
__extension__({								\
	(rng_state) *= LCG_MULTIPLIER;					\
	uint64_t __rng_val = (uint64_t)((rng_state) >> 64);		\
	__rng_val;							\
})

/**
 * @brief Computes a pseudo random number in the [0, 1] range
 * @param rng_state a test_rng_t object
 * @return a uniformly distributed pseudo random double value in [0, 1]
 */
#define lcg_random(rng_state)						\
__extension__({								\
	uint64_t __u_val = lcg_random_u(rng_state);			\
	double __ret = 0.0;						\
	if (__builtin_expect(!!__u_val, 1)) {				\
		unsigned __lzs = intrinsics_clz(__u_val) + 1;		\
		__u_val <<= __lzs;					\
		__u_val >>= 12;						\
									\
		uint64_t __exp = 1023 - __lzs;				\
		__u_val |= __exp << 52;					\
									\
		memcpy(&__ret, &__u_val, sizeof(double));		\
	}								\
	__ret;								\
})

