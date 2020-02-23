/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 *
 * This is a stripped down and adapted version for the use in NeuRome.
 * Contact first NeuRome's maintainers for issues in this code.
 */

#pragma once

#include <stdint.h>

#ifndef __SIZEOF_INT128__
#error Unsupported platform
#endif

/// We use 128 bit numbers in random number generation
typedef __uint128_t uint128_t;

#define PCG_MULTIPLIER \
	((((uint128_t)2549297995355413924ULL) << 64) + 4865540595714422341ULL)

extern uint128_t rand_inc;

// all macro arguments are of type uint128_t

// initializes the global PCG PRNG selection sequence
#define random_global_init(initseq) rand_inc = ((initseq) << 1u) | 1u

// initializes a single PCG PRNG; side effect on argument state
#define random_init(initstate, state) 					\
	(state) = (rand_inc + (initstate)) * PCG_MULTIPLIER + rand_inc

// picks a random number; side effect on state
#define random_u64(state)						\
__extension__({								\
	(state) = (state) * PCG_MULTIPLIER + rand_inc;			\
	uint64_t __val = ((uint64_t) ((state) >> 64u)) ^ (uint64_t) (state);\
	unsigned __rot = (state) >> 122u;				\
	((uint64_t)((__val >> __rot) | (__val << ((-__rot) & 63))));	\
})

#ifndef HAVE_FAST_PRNG

#define random_u01(state) ({ 						\
	int __exp = -64;						\
	uint64_t __mant;						\
									\
	while (unlikely((__mant = random_u64(rng)) == 0))		\
	{								\
		if (unlikely(__exp < -1060))				\
			return 0;					\
		__exp -= 64;						\
	}								\
									\
	unsigned __shf = __builtin_clzll(__mant);			\
	if (__shf != 0) {						\
		__exp -= __shf;						\
		__mant <<= __shf;					\
		__mant |= (random_u64(rng) >> (64 - __shf));		\
	}								\
									\
	__mant |= 1u;							\
									\
	ldexp((double)__mant, __exp);					\
})

#else

#define random_u01(state) ldexp(random_u64(state), -64)

#endif
