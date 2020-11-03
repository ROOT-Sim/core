/**
* @file mm/mm.h
*
* @brief Memory Manager main header
*
* Memory Manager main header
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
*
* @author Alessandro Pellegrini
* @author Francesco Quaglia
*/
#pragma once

#include <log/log.h>

#include <stdlib.h>
#include <stddef.h>

#ifdef NEUROME_SERIAL

#define __mm_alloc malloc
#define __mm_realloc realloc
#define __mm_free free

#else

extern void *__real_malloc(size_t mem_size);
extern void *__real_realloc(void *ptr, size_t mem_size);
extern void __real_free(void *ptr);

#define __mm_alloc __real_malloc
#define __mm_realloc __real_realloc
#define __mm_free __real_free

#endif

#ifdef NEUROME_TEST

#define mm_alloc malloc
#define mm_realloc realloc
#define mm_free free

#else

#pragma GCC poison malloc realloc free

inline void *mm_alloc(size_t mem_size)
{
	void *ret = __mm_alloc(mem_size);

	if (__builtin_expect(mem_size && !ret, 0)) {
		log_log(LOG_FATAL, "Out of memory!");
		abort();
	}
	return ret;
}

inline void *mm_realloc(void *ptr, size_t mem_size)
{
	void *ret = __mm_realloc(ptr, mem_size);

	if(__builtin_expect(mem_size && !ret, 0)) {
		log_log(LOG_FATAL, "Out of memory!");
		abort();
	}
	return ret;
}

inline void mm_free(void *ptr)
{
	__mm_free(ptr);
}

#endif
