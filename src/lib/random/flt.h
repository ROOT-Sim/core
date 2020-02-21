#pragma once

#ifdef HAVE_FAST_PRNG

#define random_u01(state) ({ 						\
	int __exp = -64;						\
	uint64_t __mant;						\
									\
	while (unlikely((__mant = pcg_random64(rng)) == 0))		\
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
		__mant |= (pcg_random64(rng) >> (64 - __shf));		\
	}								\
									\
	__mant |= 1u;							\
									\
	return ldexp((double)__mant, __exp);				\
})

#else

#define random_u01(state) ({ 						\
	return ldexp(pcg_random(state), -64);				\
})

#endif
