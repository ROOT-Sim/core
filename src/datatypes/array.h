/**
 * @file datatypes/array.h
 *
 * @brief Dynamic array datatype
 *
 * Dynamic array datatype
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <mm/mm.h>

#include <stdint.h>
#include <string.h>

/// The initial size of dynamic arrays expressed in the number of elements
#define INIT_SIZE_ARRAY 8U
/// The type used to handle dynamic arrays count of elements and capacity
typedef uint_least32_t array_count_t;

/**
 * @brief Declares a dynamic array
 * @param type The type of the contained elements
 */
#define dyn_array(type)                                                                                                \
	struct {                                                                                                       \
		type *items;                                                                                           \
		array_count_t count;                                                                                   \
		array_count_t capacity;                                                                                \
	}

/**
 * @brief Gets the underlying actual array of elements of a dynamic array
 * @param self The target dynamic array
 * @return a pointer to the underlying array of elements
 *
 * You can use the array to directly index items, but do it at your own risk!
 */
#define array_items(self) ((self).items)

/**
 * @brief Gets the count of contained element in a dynamic array
 * @param self The target dynamic array
 * @return the count of contained elements
 */
#define array_count(self) ((self).count)

/**
 * @brief Gets the current capacity of a dynamic array
 * @param self The target dynamic array
 */
#define array_capacity(self) ((self).capacity)

/**
 * @brief Gets the last element of a dynamic array
 * @param self the target dynamic array
 * @return the last element of the array
 *
 * You have to check for the array emptiness before safely calling this macro
 */
#define array_peek(self) (array_items(self)[array_count(self) - 1])

/**
 * @brief Gets the i-th element of a dynamic array
 * @param self The target dynamic array
 * @param i The index of the element to get
 *
 * @warning This function is unsafe: It is not checked if the index is valid!
 */
#define array_get_at(self, i) (array_items(self)[(i)])

/**
 * @brief Check if the dynamic array is empty
 * @param self The target dynamic array
 */
#define array_is_empty(self) (array_count(self) == 0)

/**
 * @brief Initializes a dynamic array
 * @param self The target dynamic array
 */
#define array_init(self)                                                                                               \
	__extension__({                                                                                                \
		array_capacity(self) = INIT_SIZE_ARRAY;                                                                \
		array_items(self) = mm_alloc(array_capacity(self) * sizeof(*array_items(self)));                       \
		array_count(self) = 0;                                                                                 \
	})

/**
 * @brief Releases the memory used by a dynamic array
 * @param self The target dynamic array
 */
#define array_fini(self) __extension__({ mm_free(array_items(self)); })

/**
 * @brief Push an element to the end of a dynamic array
 * @param self The target dynamic array
 * @param elem The element to push
 */
#define array_push(self, elem)                                                                                         \
	__extension__({                                                                                                \
		array_expand(self);                                                                                    \
		array_items(self)[array_count(self)] = (elem);                                                         \
		array_count(self)++;                                                                                   \
	})

/**
 * @brief Pop an element from the end of a dynamic array
 * @param self The target dynamic array
 */
#define array_pop(self)                                                                                                \
	__extension__({                                                                                                \
		array_count(self)--;                                                                                   \
		array_items(self)[array_count(self)];                                                                  \
	})

/**
 * @brief Insert an element at the given index
 * @param self The target dynamic array
 * @param i The index at which to insert the element
 * @param elem The element to insert
 */
#define array_add_at(self, i, elem)                                                                                    \
	__extension__({                                                                                                \
		array_expand(self);                                                                                    \
		memmove(&(array_items(self)[(i) + 1]), &(array_items(self)[(i)]),                                      \
		    sizeof(*array_items(self)) * (array_count(self) - (i)));                                           \
		array_items(self)[(i)] = (elem);                                                                       \
		array_count(self)++;                                                                                   \
	})

/**
 * @brief Remove an element at the given index from the dynamic array
 * @param self The target dynamic array
 * @param i The index of the element to remove
 */
#define array_remove_at(self, i)                                                                                       \
	__extension__({                                                                                                \
		__typeof__(*array_items(self)) __rmval;                                                                \
		array_count(self)--;                                                                                   \
		__rmval = array_items(self)[(i)];                                                                      \
		memmove(&(array_items(self)[(i)]), &(array_items(self)[(i) + 1]),                                      \
		    sizeof(*array_items(self)) * (array_count(self) - (i)));                                           \
		array_shrink(self);                                                                                    \
		__rmval;                                                                                               \
	})

/**
 * @brief Remove first n elements from the dynamic array
 * @param self The target dynamic array
 * @param n The number of elements to remove
 */
#define array_truncate_first(self, n)                                                                                  \
	__extension__({                                                                                                \
		array_count(self) -= (n);                                                                              \
		memmove(array_items(self), &(array_items(self)[n]), sizeof(*array_items(self)) * (array_count(self))); \
	})

/**
 * @brief Reduce the size of the dynamic array
 * @param self The target dynamic array
 *
 * The size of the dinamic array is halved if the number of elements is less than a third of the capacity.
 */
#define array_shrink(self)                                                                                             \
	__extension__({                                                                                                \
		if(unlikely(array_count(self) > INIT_SIZE_ARRAY && array_count(self) * 3 <= array_capacity(self))) {   \
			array_capacity(self) /= 2;                                                                     \
			array_items(self) =                                                                            \
			    mm_realloc(array_items(self), array_capacity(self) * sizeof(*array_items(self)));          \
		}                                                                                                      \
	})

/**
 * @brief Expand the size of the dynamic array to have at least the requested capacity
 * @param self The target dynamic array
 * @param n The requested capacity
 */
#define array_reserve(self, n)                                                                                         \
	__extension__({                                                                                                \
		__typeof__(array_count(self)) tcnt = array_count(self) + (n);                                          \
		if(unlikely(tcnt >= array_capacity(self))) {                                                           \
			do {                                                                                           \
				array_capacity(self) *= 2;                                                             \
			} while(unlikely(tcnt >= array_capacity(self)));                                               \
			array_items(self) =                                                                            \
			    mm_realloc(array_items(self), array_capacity(self) * sizeof(*array_items(self)));          \
		}                                                                                                      \
	})

/**
 * @brief Double the size of the dynamic array if the number of elements is greater than the capacity
 * @param self The target dynamic array
 */
#define array_expand(self)                                                                                             \
	__extension__({                                                                                                \
		if(unlikely(array_count(self) >= array_capacity(self))) {                                              \
			array_capacity(self) *= 2;                                                                     \
			array_items(self) =                                                                            \
			    mm_realloc(array_items(self), array_capacity(self) * sizeof(*array_items(self)));          \
		}                                                                                                      \
	})

/**
 * @brief Remove an element at the given index from the dynamic array, last array element will take its place
 * @param self The target dynamic array
 * @param i The index of the element to remove
 */
#define array_lazy_remove_at(self, i)                                                                                  \
	__extension__({ array_items(self)[(i)] = array_items(self)[--array_count(self)]; })
