#pragma once

#include <core/intrinsics.h>

#include <stdint.h>

typedef __uint128_t uint128_t;

#define LCG_MULTIPLIER (\
	(((uint128_t)0x0fc94e3bf4e9ab32ULL) << 64) + 0x866458cd56f5e605ULL)

// all macro arguments are of type uint128_t
// these macros do side effects on the passed arguments

#define lcg_init(rng_state, initseq) (rng_state) = ((initseq) << 1u) | 1u

#define lcg_random_u(rng_state)						\
__extension__ ({							\
	(rng_state) *= LCG_MULTIPLIER;					\
	uint64_t __rng_val = (uint64_t)((rng_state) >> 64);		\
	__rng_val;							\
})

// this is not extremely good; it assumes a specific double endianness
// ldexp() can't be confidently used since compilers embed different versions
#define lcg_random(rng_state)						\
__extension__ ({							\
	uint64_t __u_val = lcg_random_u(rng_state);			\
	double __ret = 0.0;						\
	if (__builtin_expect(!!__u_val, 1)) {				\
		unsigned __lzs = SAFE_CLZ(__u_val) + 1;			\
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

