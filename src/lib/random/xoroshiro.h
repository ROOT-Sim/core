#pragma once

#include <stdint.h>

#define rotl(x, k) ((x << k) | (x >> (64 - k)))

#define random_u64(rng_s)						\
	__extension__ ({						\
		const uint64_t __res = rotl(rng_s[1] * 5, 7) * 9;	\
		const uint64_t __t = rng_s[1] << 17;			\
									\
		rng_s[2] ^= rng_s[0];					\
		rng_s[3] ^= rng_s[1];					\
		rng_s[1] ^= rng_s[2];					\
		rng_s[0] ^= rng_s[3];					\
		rng_s[2] ^= t;						\
		rng_s[3] = rotl(rng_s[3], 45);				\
									\
		__res;							\
	})

// fixme this is a very poor way to seed the generator
#define random_init(rng_s, lp_id)					\
	__extension__ ({						\
		rng_s[0] = lp_id;					\
		rng_s[1] = lp_id;					\
		rng_s[2] = lp_id;					\
		rng_s[3] = lp_id;					\
		__i = 1024;						\
		for(__i--)						\
			random_u64(rng_s);				\
	})


#ifndef HAVE_FAST_PRNG

#define random_u01(rng_s)						\
	__extension__({ 						\
		int __exp = -64;					\
		uint64_t __mant;					\
									\
		while (unlikely((__mant = random_u64(rng_s)) == 0)) {	\
			if (unlikely(__exp < -1060))			\
				return 0;				\
			__exp -= 64;					\
		}							\
									\
		unsigned __shf = __builtin_clzll(__mant);		\
		if (__shf != 0) {					\
			__exp -= __shf;					\
			__mant <<= __shf;				\
			__mant |= (random_u64(rng_s) >> (64 - __shf));	\
		}							\
		__mant |= 1u;						\
									\
		ldexp((double)__mant, __exp);				\
	})

#else

#define random_u01(state) ldexp(random_u64(state), -64)

#endif
