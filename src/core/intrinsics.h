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

