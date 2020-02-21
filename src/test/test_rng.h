#pragma once

#include <math.h>
#include <stdint.h>

typedef __uint128_t uint128_t;

#define LCG_MULTIPLIER (\
	(((uint128_t)0x0fc94e3bf4e9ab32) << 64) + 0x866458cd56f5e605)

// all macro arguments are of type uint128_t
// these macros do side effects on the passed arguments

#define lcg_init(rng_state, initseq) (rng_state) = ((initseq) << 1u) | 1u

#define lcg_random_u(rng_state)						\
	__extension__ ({						\
		(rng_state) *= LCG_MULTIPLIER;				\
		uint64_t __rng_val = ((uint64_t)((rng_state) >> 64));	\
		__rng_val;						\
	})

#define lcg_random(rng_state)						\
	__extension__ ({						\
		(double)ldexp(lcg_random_u(rng_state), -64);		\
	})
