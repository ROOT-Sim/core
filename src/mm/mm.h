/**
 * @file mm/mm.h
 *
 * @brief Memory Manager main header
 *
 * Memory Manager main header
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <log/log.h>

#include <stddef.h>
#include <stdlib.h>

/**
 * @brief A version of the stdlib malloc() used internally
 * @param mem_size the size of the requested memory allocation
 * @return a pointer to the newly allocated memory area
 *
 * In case of memory allocation failure, a log message is taken and the
 * simulation is promptly aborted
 */
inline void *mm_alloc(size_t mem_size)
{
	void *ret = malloc(mem_size);

	if (__builtin_expect(mem_size && !ret, 0)) {
		log_log(LOG_FATAL, "Out of memory!");
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
 * In case of memory allocation failure, a log message is taken and the
 * simulation is promptly aborted
 */
inline void *mm_realloc(void *ptr, size_t mem_size)
{
	void *ret = realloc(ptr, mem_size);

	if(__builtin_expect(mem_size && !ret, 0)) {
		log_log(LOG_FATAL, "Out of memory!");
		abort(); // TODO: this can be criticized as xmalloc() in gcc. We shall dump partial stats before.
	}
	return ret;
}

/**
 * @brief A version of the stdlib free() used internally
 * @param ptr a pointer to the memory area to free
 */
inline void mm_free(void *ptr)
{
	free(ptr);
}
