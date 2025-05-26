/**
 * @file mm/mm.h
 *
 * @brief Memory Manager main header
 *
 * Memory Manager main header
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <log/log.h>

#include <arch/mem.h>
#include <core/core.h>

#include <stddef.h>
#include <stdlib.h>

/**
 * @brief A version of the stdlib aligned_alloc() used internally
 * @param alignment the requested alignment value in bytes
 * @param mem_size the size of the requested memory allocation
 * @return a pointer to the newly allocated memory area
 *
 * In case of memory allocation failure, a logger message is taken and the
 * simulation is promptly aborted
 */
static inline void *mm_aligned_alloc(size_t alignment, size_t mem_size)
{
	void *ret = mem_aligned_alloc(alignment, mem_size);

	if(unlikely(mem_size && ret == NULL)) {
		logger(LOG_FATAL, "%s", "Out of memory!");
		abort(); // TODO: this can be criticized as xmalloc() in gcc. We shall dump partial stats before.
	}
	return ret;
}

/**
 * @brief A version of the stdlib free() for aligned blocks used internally
 * @param ptr a pointer to the memory area to free
 *
 * Needed because Windows has a separate function to handle the release of
 * memory allocated with its non standard _aligned_malloc()
 */
static inline void mm_aligned_free(void *ptr)
{
	mem_aligned_free(ptr);
}


/**
 * @brief A version of the stdlib malloc() used internally
 * @param mem_size the size of the requested memory allocation
 * @return a pointer to the newly allocated memory area
 *
 * In case of memory allocation failure, a logger message is taken and the
 * simulation is promptly aborted
 */
static inline void *mm_alloc(size_t mem_size)
{
	void *ret = malloc(mem_size);

	if(unlikely(mem_size && !ret)) {
		logger(LOG_FATAL, "Out of memory!");
		abort(); // TODO: this can be criticized as xmalloc() in gcc. We shall dump partial stats before.
	}
	return ret;
}

/**
 * @brief A version of the stdlib realloc() used internally
 * @param ptr a pointer to the memory area to reallocate
 * @param mem_size the new size of the memory allocation
 * @return a pointer to the newly allocated memory area
 *
 * In case of memory allocation failure, a logger message is taken and the
 * simulation is promptly aborted
 */
static inline void *mm_realloc(void *ptr, size_t mem_size)
{
	void *ret = realloc(ptr, mem_size);

	if(unlikely(mem_size && !ret)) {
		logger(LOG_FATAL, "Out of memory!");
		abort(); // TODO: this can be criticized as xmalloc() in gcc. We shall dump partial stats before.
	}
	return ret;
}

/**
 * @brief A version of the stdlib free() used internally
 * @param ptr a pointer to the memory area to free
 */
static inline void mm_free(void *ptr)
{
	free(ptr);
}
