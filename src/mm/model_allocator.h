/**
 * @file mm/model_allocator.h
 *
 * @brief Memory management functions for simulation models
 *
 * Memory management functions for simulation models
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>
#include <mm/buddy/buddy.h>

extern void model_allocator_init(void);
extern void model_allocator_fini(void);
extern void model_allocator_lp_init(void);
extern void model_allocator_lp_fini(void);
extern void model_allocator_checkpoint_take(array_count_t ref_i);
extern void model_allocator_checkpoint_next_force_full(void);
extern array_count_t model_allocator_checkpoint_restore(array_count_t ref_i);
extern array_count_t model_allocator_fossil_lp_collect(struct mm_state *self,
		array_count_t tgt_ref_i);

extern void __write_mem(const void *ptr, size_t siz);

extern void *malloc_mt(size_t req_size);
extern void *calloc_mt(size_t nmemb, size_t req_size);
extern void free_mt(void *ptr);
extern void *realloc_mt(void *ptr, size_t req_size);
