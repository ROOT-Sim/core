/**
* @file mm/model_allocator.h
*
* @brief Memory management functions for simulation models
*
* Memory management functions for simulation models
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
*/
#pragma once

#include <datatypes/array.h>

#include <mm/buddy/buddy.h>

extern void model_allocator_lp_init(void);
extern void model_allocator_lp_fini(void);
extern void model_allocator_checkpoint_take(array_count_t ref_i);
extern void model_allocator_checkpoint_next_force_full(void);
extern array_count_t model_allocator_checkpoint_restore(array_count_t ref_i);
extern array_count_t model_allocator_fossil_lp_collect(array_count_t tgt_ref_i);

extern void __write_mem(void *ptr, size_t siz);

extern void *__wrap_malloc(size_t req_size);
extern void *__wrap_calloc(size_t nmemb, size_t req_size);
extern void __wrap_free(void *ptr);
extern void *__wrap_realloc(void *ptr, size_t req_size);
