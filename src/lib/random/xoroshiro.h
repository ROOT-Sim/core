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

#define random_u64(rng_s)						\
__extension__ ({							\
	const uint64_t __res = rotl((rng_s)[1] * 5, 7) * 9;		\
	const uint64_t __t = (rng_s)[1] << 17;				\
									\
	(rng_s)[2] ^= (rng_s)[0];					\
	(rng_s)[3] ^= (rng_s)[1];					\
	(rng_s)[1] ^= (rng_s)[2];					\
	(rng_s)[0] ^= (rng_s)[3];					\
	(rng_s)[2] ^= __t;						\
	(rng_s)[3] = rotl((rng_s)[3], 45);				\
									\
	__res;								\
})

// FIXME: this is a very poor way to seed the generator
#define random_init(rng_s, llid, seed)					\
__extension__ ({							\
	(rng_s)[0] = (llid + 1) * UINT64_C(16232384076195101791) ^ seed;\
	(rng_s)[1] = (llid + 1) * UINT64_C(13983006573105492179) ^ seed;\
	(rng_s)[2] = (llid + 1) * UINT64_C(10204677566545858177) ^ seed;\
	(rng_s)[3] = (llid + 1) * UINT64_C(14539058011249359317) ^ seed;\
	unsigned __i = 1024;						\
	while (__i--)							\
		random_u64((rng_s));					\
})

