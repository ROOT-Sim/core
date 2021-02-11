/**
* @file lib/random/xoroshiro.h
*
* @brief Xoroshiro RNG support functions
*
* Xoroshiro RNG support functions.
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
* @todo Does it make sense to have this in a separate header?
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
#define random_init(rng_s, llid)					\
__extension__ ({							\
	(rng_s)[0] = (llid + 1) * UINT64_C(16232384076195101791);	\
	(rng_s)[1] = (llid + 1) * UINT64_C(13983006573105492179);	\
	(rng_s)[2] = (llid + 1) * UINT64_C(10204677566545858177);	\
	(rng_s)[3] = (llid + 1) * UINT64_C(14539058011249359317);	\
	unsigned __i = 1024;						\
	while (__i--)							\
		random_u64((rng_s));					\
})

