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

#define retractable_heap_declare(type) dyn_array(type)

#define retractable_heap_parent_i(i) (((i) - 1) / HEAP_K)
#define retractable_heap_child_i(i) ((i) * HEAP_K + 1)

/**
 * @brief Initializes an empty heap
 * @param self the heap to initialize
 */
#define retractable_heap_init(self) array_init(self)

/**
 * @brief Finalizes an heap
 * @param self the heap to finalize
 *
 * The user is responsible for cleaning up the possibly contained items.
 */
#define retractable_heap_fini(self) array_fini(self)

#define retractable_heap_is_empty(self) array_is_empty(self)
#define retractable_heap_min(self) (*(__typeof__(*array_items(self)) *const)array_items(self))

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
#define retractable_heap_insert(self, cmp_f, upd_f, elem)                                                              \
	__extension__({                                                                                                \
		array_expand(self);                                                                                    \
		__typeof__(array_count(self)) k = array_count(self)++;                                                 \
		__typeof__(array_items(self)) items = array_items(self);                                               \
		while(k && cmp_f(elem, items[retractable_heap_parent_i(k)])) {                                         \
			items[k] = items[retractable_heap_parent_i(k)];                                                \
			upd_f(items[k], k);                                                                            \
			k = retractable_heap_parent_i(k);                                                              \
		}                                                                                                      \
		items[k] = elem;                                                                                       \
		upd_f(items[k], k);                                                                                    \
		k;                                                                                                     \
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
#define retractable_heap_extract(self, cmp_f, upd_f)                                                                   \
	__extension__({                                                                                                \
		__typeof__(array_items(self)) items = array_items(self);                                               \
		__typeof__(*array_items(self)) reth = items[0], lasth = array_pop(self);                               \
		__typeof__(array_count(self)) p = 0U, i = 1U, mh;                                                      \
__bubble_up__:                                                                                                         \
		mh = min(i + HEAP_K, array_count(self));                                                               \
		while(i < mh) {                                                                                        \
			if(cmp_f(items[i], lasth)) {                                                                   \
				__typeof__(array_count(self)) j = i + 1;                                               \
				while(j < mh) {                                                                        \
					if(cmp_f(items[j], items[i]))                                                  \
						i = j;                                                                 \
					++j;                                                                           \
				}                                                                                      \
				items[p] = items[i];                                                                   \
				upd_f(items[p], p);                                                                    \
				p = i;                                                                                 \
				i = retractable_heap_child_i(i);                                                       \
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
#define retractable_heap_priority_increased(self, cmp_f, upd_f, elem, pos)                                             \
	__extension__({                                                                                                \
		__typeof__(array_items(self)) items = array_items(self);                                               \
		__typeof__(array_count(self)) p = (pos), k = retractable_heap_parent_i(p);                             \
		while(p && cmp_f((elem), items[k])) {                                                                  \
			items[p] = items[k];                                                                           \
			upd_f(items[p], p);                                                                            \
			p = k;                                                                                         \
			k = retractable_heap_parent_i(p);                                                              \
		}                                                                                                      \
		items[p] = (elem);                                                                                     \
		upd_f(items[p], p);                                                                                    \
	})

#define retractable_heap_priority_decreased(self, cmp_f, upd_f, elem, pos)                                             \
	__extension__({                                                                                                \
		__typeof__(array_items(self)) items = array_items(self);                                               \
		__typeof__(array_count(self)) p = (pos), k = retractable_heap_child_i(p), mh;                          \
		mh = min(k + HEAP_K, array_count(self));                                                               \
		while(k < mh) {                                                                                        \
			if(!cmp_f(items[k], (elem))) {                                                                 \
				++k;                                                                                   \
				continue;                                                                              \
			}                                                                                              \
			__typeof__(array_count(self)) j = k + 1;                                                       \
			while(j < mh) {                                                                                \
				if(cmp_f(items[j], items[k]))                                                          \
					k = j;                                                                         \
				++j;                                                                                   \
			}                                                                                              \
			items[p] = items[k];                                                                           \
			upd_f(items[p], p);                                                                            \
			p = k;                                                                                         \
			k = retractable_heap_child_i(k);                                                               \
			mh = min(k + HEAP_K, array_count(self));                                                       \
		}                                                                                                      \
		items[p] = (elem);                                                                                     \
		upd_f(items[p], p);                                                                                    \
	})
