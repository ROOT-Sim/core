/**
 * @file datatypes/vector.c
 *
 * @brief Dynamic array datatype of buffers of a given size
 *
 * Dynamic array datatype of buffers of a given size.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <datatypes/vector.h>

/**
 * @brief A struct to implement a resizable vector of unknown types.
 *
 * This is an opaque structure. It can be used at runtime for unknown types, as only the size in bytes
 * of the elements of the vector are required.
 */
struct vector {
	size_t count; ///< Currently used slots
	size_t slots; ///< Total allocated slots
	size_t size; ///< Size in bytes of a slot
	void *arr; /// A pointer to the actual array
};

/**
 * @brief Resize a vector if the capacity needs to change
 *
 * @param arr The vector to resize
 * @param new_size The new size in bytes
 * @return true on success, false upon error
 */
static inline bool resize(struct vector *arr, size_t new_size)
{
	assert(arr->count < new_size);
	arr->arr = realloc(arr->arr, new_size * arr->size);
	if(!arr->arr) {
		return false;
	}
	arr->slots = new_size;
	return true;
}

/**
 * @brief Initialize an opaque struct vector
 *
 * @param nmemb Number of elements to setup the initial capacity
 * @param size Size of an element
 * @return A pointer to the newlt-allocated resizable vector
 */
void *init_vector(size_t nmemb, size_t size)
{
	struct vector *ret;
	void *p;

	// Check for overflow
	if(size && nmemb > (size_t) -1 / size) {
		return NULL;
	}

	p = malloc(size * nmemb);
	if(!p) {
		return p;
	}
	memset(p, 0, nmemb * size);

	ret = malloc(sizeof(*ret));
	if(!ret) {
		free(p);
		return ret;
	}

	ret->count = 0;
	ret->slots = nmemb;
	ret->size = size;
	ret->arr = p;

	return ret;
}

/**
 * @brief Free an opaque struct vector
 * @param arr The resizable vector to release
 */
void free_vector(struct vector *arr)
{
	free(arr->arr);
	free(arr);
}

/**
 * @brief Push an element at the end of the resizable vector
 *
 * A copy of the element is made.
 *
 * @param arr The resizable vector to push the element into
 * @param el A pointer to the element to push back
 * @return true on sucess, false on error
 */
bool push_back(struct vector *arr, void *el)
{
	return insert_at(arr, arr->count, el);
}

/**
 * @brief Insert an element at a certain position, and move subsequent elements accordingly.
 *
 * A copy of the element is made.
 * It is not possible to leave holes: the last value accepted as pos is the next-to-last element in the vector.
 *
 * @param arr The resizable vector to push the element into
 * @param pos The position in the vector where to insert the new element
 * @param el A pointer to the element to push back
 * @return true on sucess, false on error
 */
bool insert_at(struct vector *arr, size_t pos, void *el)
{
	char *from, *to;
	size_t size;

	if(pos > arr->count) {
		return false;
	}

	if(arr->count + 1 > arr->slots) {
		if(!resize(arr, arr->slots * 2)) {
			return false;
		}
	}

	from = (char *) arr->arr + pos * arr->size;
	if(pos + 1 < arr->slots) {
		to = from + arr->size;
		size = (arr->count - pos) * arr->size;
		memmove(to, from, size);
	}

	memcpy(from, el, arr->size);

	arr->count++;
	assert(arr->count <= arr->slots);
	return true;
}

/**
 * @brief Get an element at a certain position and make a copy in buffer
 *
 * @param arr The resizable vector
 * @param pos The element in the vector
 * @param buffer A buffer (of suitable size) where to copy the element
 * @return true on success, false on error
 */
bool read_from(struct vector *arr, size_t pos, void *buffer)
{
	if(pos < arr->count) {
		memcpy(buffer, (char *) arr->arr + pos * arr->size, arr->size);
		return true;
	}

	return false;
}

/**
 * @brief Copy an element in buffer and remove it from the array
 *
 * @param arr The resizable vector
 * @param pos The element in the vector
 * @param buffer A buffer (of suitable size) where to copy the element
 * @return true on success, false on error
 */
bool extract_from(struct vector *arr, size_t pos, void *buffer)
{
	char *from, *to;
	size_t size;

	if(!read_from(arr, pos, buffer)) {
		return false;
	}

	if(pos + 1 < arr->slots) {
		from = (char *) arr->arr + (pos + 1) * arr->size;
		to = from - arr->size;
		size = (arr->count - pos) * arr->size;
		memmove(to, from, size);
	}

	arr->count--;
	if(arr->count < arr->slots / 2) {
		resize(arr, arr->slots / 2);
	}
	return true;
}

/**
 * @brief Convert the resizable vector in a dynamically-allcoated array
 *
 * After calling this function, the resizable vector is freed and cannot be used anymore.
 * The freezed array is located in the heap.
 *
 * @param arr The resizable vector
 * @return A pointer to a dynamically-allocated array keeping all the elements
 */
extern void *freeze_vector(struct vector *arr)
{
	void *freezed = malloc(arr->count * arr->size);
	memcpy(freezed, arr->arr, arr->count * arr->size);
	free_vector(arr);
	return freezed;
}
