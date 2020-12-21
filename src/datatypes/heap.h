/**
* @file datatypes/heap.h
*
* @brief Heap datatype
*
* A very simple binary heap implemented on top of our dynamic array
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

#include <datatypes/array.h>

#define binary_heap(type) dyn_array(type)

#define heap_items(self) array_items(self)
#define heap_count(self) array_count(self)

/**
 * @brief Initializes an empty heap
 * @param self the heap to initialize
 */
#define heap_init(self)	array_init(self)

/**
 * @brief Finalizes an heap
 * @param self the heap to finalize
 *
 * The user is responsible for cleaning up the possibly contained items.
 */
#define heap_fini(self)	array_fini(self)

#define heap_is_empty(self) array_is_empty(self)
#define heap_min(self) ((__typeof(*array_items(self)) const)(array_items(self)[0]))

/**
 * @brief Inserts an element into the heap
 * @param self the heap target of the insertion
 * @param cmp_f a comparing function f(a, b) which returns true iff a < b
 * @param elem the element to insert
 * @returns the position of the inserted element in the underlying array
 *
 * For correct operation of the heap you need to always pass the same @a cmp_f,
 * both for insertion and extraction
 */
#define heap_insert(self, cmp_f, elem)					\
__extension__({								\
	__typeof(array_count(self)) __i_h = array_count(self);		\
	array_push(self, elem);						\
	while(								\
		__i_h && 						\
		cmp_f(elem, array_items(self)[(__i_h - 1U) / 2U])	\
	){								\
		array_items(self)[__i_h] = 				\
			array_items(self)[(__i_h - 1U) / 2U];		\
		__i_h = (__i_h - 1U) / 2U;				\
	}								\
	array_items(self)[__i_h] = elem;				\
	__i_h;								\
})

/**
 * @brief Extracts an element from the heap
 * @param self the heap from where to extract the element
 * @param cmp_f a comparing function f(a, b) which returns true iff a < b
 * @returns the extracted element
 *
 * For correct operation of the heap you need to always pass the same @a cmp_f
 * both for insertion and extraction
 */
#define heap_extract(self, cmp_f)					\
__extension__({								\
	__typeof(*array_items(self)) __ret_h = array_items(self)[0];	\
	__typeof(*array_items(self)) __last_h = array_pop(self);	\
	__typeof(array_count(self)) __i_h = 1U;				\
	while (__i_h < array_count(self)) {				\
		__i_h += __i_h + 1 < array_count(self) &&		\
			cmp_f(						\
				array_items(self)[__i_h + 1U],		\
				array_items(self)[__i_h]		\
			);						\
		if (!cmp_f(array_items(self)[__i_h], __last_h)) {	\
			break;						\
		}							\
		array_items(self)[(__i_h - 1U) / 2U] =			\
			array_items(self)[__i_h];			\
		__i_h = __i_h * 2U + 1U;				\
	}								\
	array_items(self)[(__i_h - 1U) / 2U] = __last_h;		\
	__ret_h;							\
})
