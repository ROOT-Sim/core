/**
 * @file datatypes/heap.h
 *
 * @brief Heap datatype
 *
 * A very simple binary heap implemented on top of our dynamic array
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>

/**
 * @brief Declares a heap
 * @param type the type of the contained elements
 */
#define heap_declare(type) dyn_array(type)

/**
 * @brief Gets the underlying actual array of elements of a binary heap
 * @param self the target heap
 * @return a pointer to the underlying array of elements
 *
 * You can use the returned array to directly index items, but do it at your own risk!
 */
#define heap_items(self) array_items(self)

/**
 * @brief Gets the count of contained element in a heap
 * @param self the target heap
 * @return the count of contained elements
 */
#define heap_count(self) array_count(self)

/**
 * @brief Initialize an empty heap
 * @param self the heap to initialize
 */
#define heap_init(self) array_init(self)

/**
 * @brief Finalize a heap
 * @param self the heap to finalize
 *
 * The user is responsible for cleaning up the possibly contained items.
 */
#define heap_fini(self) array_fini(self)

/**
 * @brief Check if a heap is empty
 * @param self the heap to check
 * @return true if @p self heap is empty, false otherwise
 */
#define heap_is_empty(self) array_is_empty(self)

/**
 * @brief Get the highest priority element
 * @param self the heap
 * @return the highest priority element, cast to const
 */
#define heap_min(self) (*(__typeof__ (*array_items(self)) *const)array_items(self))

/**
 * @brief Insert an element into the heap
 * @param self the heap target of the insertion
 * @param cmp_f a comparing function f(a, b) which returns true iff a < b
 * @param elem the element to insert
 * @returns the position of the inserted element in the underlying array
 *
 * For correct operation of the heap you need to always pass the same @a cmp_f, both for insertion and extraction
 */
#define heap_insert(self, cmp_f, elem)                                                                                 \
	__extension__({                                                                                                \
		array_reserve(self, 1);                                                                                \
		__typeof__ (array_count(self)) i = array_count(self)++;                                                   \
		__typeof__ (array_items(self)) items = array_items(self);                                               \
		while(i && cmp_f(elem, items[(i - 1U) / 2U])) {                                                        \
			items[i] = items[(i - 1U) / 2U];                                                               \
			i = (i - 1U) / 2U;                                                                             \
		}                                                                                                      \
		items[i] = elem;                                                                                       \
		i;                                                                                                     \
	})

/**
 * @brief Insert n elements into the heap
 * @param self the heap target of the insertion
 * @param cmp_f a comparing function f(a, b) which returns true iff a < b
 * @param ins the set of elements to insert
 * @param n the number of elements in the set
 * @returns the position of the inserted element in the underlying array
 *
 * For correct operation of the heap you need to always pass the same @a cmp_f, both for insertion and extraction
 */
#define heap_insert_n(self, cmp_f, ins, n)                                                                             \
	__extension__({                                                                                                \
		array_reserve(self, n);                                                                                \
		__typeof__ (array_count(self)) j = n;                                                                     \
		__typeof__ (array_items(self)) items = array_items(self);                                               \
		while(j--) {                                                                                           \
			__typeof__ (array_count(self)) i = array_count(self)++;                                           \
			while(i && cmp_f((ins)[j], items[(i - 1U) / 2U])) {                                            \
				items[i] = items[(i - 1U) / 2U];                                                       \
				i = (i - 1U) / 2U;                                                                     \
			}                                                                                              \
			items[i] = (ins)[j];                                                                           \
		}                                                                                                      \
	})

/**
 * @brief Extract an element from the heap
 * @param self the heap from where to extract the element
 * @param cmp_f a comparing function f(a, b) which returns true iff a < b
 * @returns the extracted element
 *
 * For correct operation of the heap you need to always pass the same @a cmp_f both for insertion and extraction
 */
#define heap_extract(self, cmp_f)                                                                                      \
	__extension__({                                                                                                    \
		__typeof__ (array_items(self)) items = array_items(self);                                                      \
		__typeof__ (*array_items(self)) ret = array_items(self)[0];                                                      \
		__typeof__ (*array_items(self)) last = array_pop(self);                                                          \
		__typeof__ (array_count(self)) cnt = array_count(self);                                                          \
		__typeof__ (array_count(self)) i = 1U;                                                                           \
		__typeof__ (array_count(self)) j = 0U;                                                                           \
		while(i < cnt) {                                                                                               \
			i += i + 1 < cnt && cmp_f(items[i + 1U], items[i]);                                                        \
			if(!cmp_f(items[i], last))                                                                                 \
				break;                                                                                                 \
			items[j] = items[i];                                                                                       \
			j = i;                                                                                                     \
			i = i * 2U + 1U;                                                                                           \
		}                                                                                                              \
		items[j] = last;                                                                                               \
		ret;                                                                                                           \
	})
