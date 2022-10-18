/**
 * @file lib/random/xoroshiro.h
 *
 * @brief Xoroshiro RNG support functions
 *
 * Xoroshiro RNG support functions.
 *
 * SPDX-FileCopyrightText: 2008-2021 D. Blackman and S. Vigna <vigna@acm.org>
 * SPDX-License-Identifier: CC0-1.0
 */
#pragma once

#include <stdint.h>

#define rotl(x, k) (((x) << (k)) | ((x) >> (64 - (k))))

#define random_u64(rng_s)                                                                                              \
	__extension__({                                                                                                \
		const uint64_t __res = rotl((rng_s)[1] * 5, 7) * 9;                                                    \
		const uint64_t __t = (rng_s)[1] << 17;                                                                 \
                                                                                                                       \
		(rng_s)[2] ^= (rng_s)[0];                                                                              \
		(rng_s)[3] ^= (rng_s)[1];                                                                              \
		(rng_s)[1] ^= (rng_s)[2];                                                                              \
		(rng_s)[0] ^= (rng_s)[3];                                                                              \
		(rng_s)[2] ^= __t;                                                                                     \
		(rng_s)[3] = rotl((rng_s)[3], 45);                                                                     \
                                                                                                                       \
		__res;                                                                                                 \
	})

