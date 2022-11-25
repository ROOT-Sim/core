/**
 * @file mm/model_allocator.h
 *
 * @brief Memory management functions for simulation models
 *
 * Memory management functions for simulation models
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stddef.h>

#include <mm/buddy/multi_buddy.h>
#include <mm/dymelor/dymelor.h>

struct mm_state {
	union {
		uint_fast32_t used_mem;
		struct multi_buddy_state m_mb;
		struct dymelor_state m_dy;
	};
};
_Static_assert(offsetof(struct mm_state, used_mem) == offsetof(struct multi_buddy_state, used_mem),
    "Can't access used memory field in an homogeneous way!");
_Static_assert(offsetof(struct multi_buddy_state, used_mem) == offsetof(struct dymelor_state, used_mem),
    "Can't access used memory field in an homogeneous way!");

#define model_allocator_state_size(self) ((self)->used_mem)

extern void model_allocator_lp_init(struct mm_state *self);
extern void model_allocator_lp_fini(struct mm_state *self);
extern void model_allocator_checkpoint_take(struct mm_state *self, array_count_t ref_i);
extern void model_allocator_checkpoint_next_force_full(struct mm_state *self);
extern array_count_t model_allocator_checkpoint_restore(struct mm_state *self, array_count_t ref_i);
extern array_count_t model_allocator_fossil_lp_collect(struct mm_state *self, array_count_t tgt_ref_i);
