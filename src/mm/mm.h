/**
* @file mm/mm.h
*
* @brief Memory Manager main header
*
* Memory Manager main header
*
* @copyright
* Copyright (C) 2008-2021 HPDCS Group
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
*
* @author Alessandro Pellegrini
* @author Francesco Quaglia
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
