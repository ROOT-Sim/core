/**
* @file core/intrinsics.h
*
* @brief Easier access to compiler extensions
*
* This header defines some macros which allow to easily rely on some
* compiler optimizations which can be used to produce more efficient
* code.
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

/**
 * @brief Counts the trailing zeros in a base 2 number
 * @param x the number on which to compute this operation
 * @return the count of trailing zeros in the base 2 representation of @a x
 *
 * The argument @a x may be of one of the supported types of the underlying
 * compiler built-ins. Selection of the matching built-in is done statically at
 * compile time. If no matching built-in is found a compilation error is thrown.
 */
#define intrinsics_ctz(x) 						\
__extension__({								\
	__builtin_choose_expr( 						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned), 	\
		__builtin_ctz(x),					\
	__builtin_choose_expr(						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned long),	\
		__builtin_ctzl(x),					\
	__builtin_choose_expr(						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned long long),\
		__builtin_ctzll(x),					\
	(void)0)));							\
})

/**
 * @brief Counts the leading zeros in a base 2 number
 * @param x the number on which to compute this operation
 * @return the count of leading zeros in the base 2 representation of @a x
 *
 * The argument @a x may be of one of the supported types of the underlying
 * compiler built-ins. Selection of the matching built-in is done statically at
 * compile time. If no matching built-in is found a compilation error is thrown.
 */
#define intrinsics_clz(x) 						\
__extension__({								\
	__builtin_choose_expr( 						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned), 	\
		__builtin_clz(x),					\
	__builtin_choose_expr(						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned long),	\
		__builtin_clzl(x),					\
	__builtin_choose_expr(						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned long long),\
		__builtin_clzll(x),					\
	(void)0)));							\
})

/**
 * @brief Counts the set bits in a base 2 number
 * @param x the number on which to compute this operation
 * @return the count of set bits in the base 2 representation of @a x
 *
 * The argument @a x may be of one of the supported types of the underlying
 * compiler built-ins. Selection of the matching built-in is done statically at
 * compile time. If no matching built-in is found a compilation error is thrown.
 */
#define intrinsics_popcount(x) 						\
__extension__({								\
	__builtin_choose_expr( 						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned), 	\
		__builtin_popcount(x),					\
	__builtin_choose_expr(						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned long),	\
		__builtin_popcountl(x),					\
	__builtin_choose_expr(						\
	__builtin_types_compatible_p(__typeof__ (x), unsigned long long),\
		__builtin_popcountll(x),				\
	(void)0)));							\
})

