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

#include <stdlib.h>
#include <stddef.h>

#ifdef ROOTSIM_TEST

#define mm_alloc malloc
#define mm_realloc realloc
#define mm_free free

#else

#include <log/log.h>

inline void *mm_alloc(size_t mem_size)
{
	void *ret = malloc(mem_size);

	if (__builtin_expect(mem_size && !ret, 0)) {
		log_log(LOG_FATAL, "Out of memory!");
		abort(); // TODO: this can be criticized as xmalloc() in gcc. We shall dump partial stats before.
	}
	return ret;
}

inline void *mm_realloc(void *ptr, size_t mem_size)
{
	void *ret = realloc(ptr, mem_size);

	if(__builtin_expect(mem_size && !ret, 0)) {
		log_log(LOG_FATAL, "Out of memory!");
		abort(); // TODO: this can be criticized as xmalloc() in gcc. We shall dump partial stats before.
	}
	return ret;
}

inline void mm_free(void *ptr)
{
	free(ptr);
}

#pragma GCC poison malloc realloc free

#endif
