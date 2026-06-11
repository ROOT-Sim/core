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

/// The checkpointable memory context assigned to a single LP
struct mm_state {
	/// The array of pointers to the allocated buddy systems for the LP
	array_declare(struct buddy_state *) buddies;
	/// The array of checkpoints
	array_declare(struct mm_log) logs;
	/// The total count of allocated bytes
	uint_fast32_t full_ckpt_size;
	/// When set, the next checkpoint will be forced to be a full one
	bool force_full;
	/// Counter of checkpoints taken since the last full checkpoint (for full_ckpt_period)
	unsigned ckpt_since_last_full;
	/// Cache of the last buddy found by __write_mem() to avoid repeated binary searches
	struct buddy_state *last_dirty_buddy;
};

extern struct buddy_state *buddy_find_by_address(const struct mm_state *self, const void *ptr);
extern void model_allocator_lp_init(struct mm_state *self);
extern void model_allocator_lp_fini(const struct mm_state *self);
extern array_count_t model_allocator_fossil_lp_collect(struct mm_state *self, array_count_t tgt_ref_i);
