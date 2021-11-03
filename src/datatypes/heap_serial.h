/**
 * @file datatypes/heap.h
 *
 * @brief Heap datatype
 *
 * A very simple binary heap implemented on top of our dynamic array
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>

#define s_heap_declare(type) dyn_array(type)

#define s_heap_items(self) array_items(self)
#define s_heap_count(self) array_count(self)

/**
 * @brief Initializes an empty heap
 * @param self the heap to initialize
 */
#define s_heap_init(self)	array_init(self)

/**
 * @brief Finalizes an heap
 * @param self the heap to finalize
 *
 * The user is responsible for cleaning up the possibly contained items.
 */
#define s_heap_fini(self)	array_fini(self)

#define s_heap_is_empty(self) array_is_empty(self)
#define s_heap_min(self) (*(__typeof(*array_items(self)) *const)array_items(self))

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
#define s_heap_insert(self, cmp_f, elem)					\
__extension__({								\
	array_reserve(self, 1);						\
	__typeof(array_count(self)) i = array_count(self)++;		\
	__typeof__(array_items(self)) items = array_items(self);	\
	while (i && cmp_f(elem, items[(i - 1U) / 2U])) {		\
		items[i] = items[(i - 1U) / 2U];			\
		i = (i - 1U) / 2U;					\
	}								\
	items[i] = elem;						\
	i;								\
})

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
#define s_heap_insert_n(self, cmp_f, ins, n)				\
__extension__({								\
	array_reserve(self, n);						\
	__typeof(array_count(self)) j = n;				\
	__typeof__(array_items(self)) items = array_items(self);	\
	while (j--) {							\
		__typeof(array_count(self)) i = array_count(self)++;	\
		while (i && cmp_f(ins[j], items[(i - 1U) / 2U])) {	\
			items[i] = items[(i - 1U) / 2U];		\
			i = (i - 1U) / 2U;				\
		}							\
		items[i] = ins[j];					\
	}								\
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
#define s_heap_extract(self, cmp_f)					\
__extension__({								\
	__typeof__(array_items(self)) items = array_items(self);	\
	__typeof(*array_items(self)) ret = array_items(self)[0];	\
	__typeof(*array_items(self)) last = array_pop(self);		\
	__typeof(array_count(self)) cnt = array_count(self);		\
	__typeof(array_count(self)) i = 1U;				\
	__typeof(array_count(self)) j = 0U;				\
	while (i < cnt) {						\
		i += i + 1 < cnt && cmp_f(items[i + 1U], items[i]);	\
		if (!cmp_f(items[i], last)) {				\
                        break;						\
		}							\
		items[j] = items[i];					\
		j = i;							\
		i = i * 2U + 1U;					\
	}								\
	items[j] = last;						\
	ret;								\
})
