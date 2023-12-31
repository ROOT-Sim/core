/**
 * @file mm/model_allocator.h
 *
 * @brief Memory management functions for simulation models
 *
 * Memory management functions for simulation models
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>
#include <mm/buddy/multi.h>

extern void model_allocator_lp_init(struct mm_ctx *self);
extern void model_allocator_lp_fini(struct mm_ctx *self);
extern void model_allocator_checkpoint_take(struct mm_ctx *self, array_count_t ref_i);
extern array_count_t model_allocator_checkpoint_restore(struct mm_ctx *self, array_count_t ref_i);
extern array_count_t model_allocator_fossil_lp_collect(struct mm_ctx *self, array_count_t tgt_ref_i);

extern array_count_t model_allocator_serialize_size(const struct mm_ctx *self);
extern void model_allocator_serialize_dump(const struct mm_ctx *self, unsigned char *data);
extern const unsigned char *model_allocator_serialize_restore(struct mm_ctx *self, const unsigned char *data);
