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

#define HEAP_K 4

#define rheap_declare(type) dyn_array(type)

#define rheap_parent_i(i) (((i)-1) / HEAP_K)
#define rheap_child_i(i) ((i)*HEAP_K + 1)

#define rheap_items(self) array_items(self)
#define rheap_count(self) array_count(self)

/**
 * @brief Initializes an empty heap
 * @param self the heap to initialize
 */
#define rheap_init(self) array_init(self)

/**
 * @brief Finalizes an heap
 * @param self the heap to finalize
 *
 * The user is responsible for cleaning up the possibly contained items.
 */
#define rheap_fini(self) array_fini(self)

#define rheap_is_empty(self) array_is_empty(self)
#define rheap_min(self) (*(__typeof(*array_items(self)) *const)array_items(self))

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
#define rheap_insert(self, cmp_f, upd_f, elem)                                                                         \
	__extension__({                                                                                                \
		array_reserve(self, 1);                                                                                \
		__typeof(array_count(self)) i = array_count(self)++;                                                   \
		__typeof(array_items(self)) items = array_items(self);                                                 \
		while(i && cmp_f(elem, items[rheap_parent_i(i)])) {                                                    \
			items[i] = items[rheap_parent_i(i)];                                                           \
			upd_f(items[i], i);                                                                            \
			i = rheap_parent_i(i);                                                                         \
		}                                                                                                      \
		items[i] = elem;                                                                                       \
		upd_f(items[i], i);                                                                                    \
		i;                                                                                                     \
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
#define rheap_extract(self, cmp_f, upd_f)                                                                              \
	__extension__({                                                                                                \
		__typeof(array_items(self)) items = array_items(self);                                                 \
		__typeof(*array_items(self)) reth = items[0];                                                          \
		__typeof(*array_items(self)) lasth = array_pop(self);                                                  \
		__typeof(array_count(self)) p = 0U;                                                                    \
		__typeof(array_count(self)) i = 1U;                                                                    \
		__typeof(array_count(self)) mh;                                                                        \
		__bubble_up__:                                                                                         \
		mh = min(i + HEAP_K, array_count(self));                                                               \
		while(i < mh) {                                                                                        \
			if(cmp_f(items[i], lasth)) {                                                                   \
				__typeof(array_count(self)) j = i + 1;                                                 \
				while(j < mh) {                                                                        \
					if(cmp_f(items[j], items[i]))                                                  \
						i = j;                                                                 \
					++j;                                                                           \
				}                                                                                      \
				items[p] = items[i];                                                                   \
				upd_f(items[p], p);                                                                    \
				p = i;                                                                                 \
				i = rheap_child_i(i);                                                                  \
				goto __bubble_up__;                                                                    \
			}                                                                                              \
			++i;                                                                                           \
		}                                                                                                      \
		items[p] = lasth;                                                                                      \
		upd_f(items[p], p);                                                                                    \
		reth;                                                                                                  \
	})

/**
 * @brief Moves an element in the heap according to new priority
 * @param self the heap in which to move the element
 * @param cmp_f a comparing function f(a, b) which returns true iff a < b
 *
 * For correct operation of the heap you need to always pass the same @a cmp_f
 * both for insertion, extraction and priority change
 */


#define rheap_priority_increased(self, cmp_f, upd_f, elem, pos)                                                        \
	__extension__({                                                                                                \
		__typeof(array_items(self)) items = array_items(self);                                                 \
		__typeof(array_count(self)) i = pos;                                                                   \
		if(i && cmp_f(elem, items[rheap_parent_i(i)])) {                                                       \
			while(i && cmp_f(elem, items[rheap_parent_i(i)])) {                                            \
				items[i] = items[rheap_parent_i(i)];                                                   \
				upd_f(items[i], i);                                                                    \
				i = rheap_parent_i(i);                                                                 \
			}                                                                                              \
			items[i] = elem;                                                                               \
			upd_f(items[i], i);                                                                            \
		}                                                                                                      \
	})

#define rheap_priority_lowered(self, cmp_f, upd_f, elem, pos)                                                          \
	__extension__({                                                                                                \
		__typeof(array_items(self)) items = array_items(self);                                                 \
		__typeof(array_count(self)) p = pos;                                                                   \
		__typeof(array_count(self)) i = rheap_child_i(p);                                                      \
		__typeof(array_count(self)) mh;                                                                        \
		__bubble_up__:                                                                                         \
		mh = min(i + HEAP_K, array_count(self));                                                               \
		while(i < mh) {                                                                                        \
			if(cmp_f(items[i], elem)) {                                                                    \
				__typeof(array_count(self)) j = i + 1;                                                 \
				while(j < mh) {                                                                        \
					if(cmp_f(items[j], items[i]))                                                  \
						i = j;                                                                 \
					++j;                                                                           \
				}                                                                                      \
				items[p] = items[i];                                                                   \
				upd_f(items[p], p);                                                                    \
				p = i;                                                                                 \
				i = rheap_child_i(i);                                                                  \
				goto __bubble_up__;                                                                    \
			}                                                                                              \
			++i;                                                                                           \
		}                                                                                                      \
		items[p] = elem;                                                                                       \
		upd_f(items[p], p);                                                                                    \
	})
