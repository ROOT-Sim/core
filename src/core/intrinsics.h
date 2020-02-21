#pragma once
#define SAFE_CTZ(x)	(\
		__builtin_choose_expr( \
		__builtin_types_compatible_p (__typeof__ (x), unsigned), __builtin_ctz(x),\
		__builtin_choose_expr(\
		__builtin_types_compatible_p (__typeof__ (x), unsigned long), __builtin_ctzl(x),\
		__builtin_choose_expr(\
		__builtin_types_compatible_p (__typeof__ (x), unsigned long long), __builtin_ctzll(x),\
		(void)0))))

#define SAFE_CLZ(x)	(\
		__builtin_choose_expr( \
		__builtin_types_compatible_p (__typeof__ (x), unsigned), __builtin_clz(x),\
		__builtin_choose_expr(\
		__builtin_types_compatible_p (__typeof__ (x), unsigned long), __builtin_clzl(x),\
		__builtin_choose_expr(\
		__builtin_types_compatible_p (__typeof__ (x), unsigned long long), __builtin_clzll(x),\
		(void)0))))

#define SAFE_POPC(x)	(\
		__builtin_choose_expr( \
		__builtin_types_compatible_p (__typeof__ (x), unsigned), __builtin_popcount(x),\
		__builtin_choose_expr(\
		__builtin_types_compatible_p (__typeof__ (x), unsigned long), __builtin_popcountl(x),\
		__builtin_choose_expr(\
		__builtin_types_compatible_p (__typeof__ (x), unsigned long long), __builtin_popcountll(x),\
		(void)0))))

#define FETCH_AND_ADD(x_ptr, increment) __sync_fetch_and_add(x_ptr, increment)
