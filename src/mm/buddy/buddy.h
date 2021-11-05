/**
 * @file mm/buddy/buddy.h
 *
 * @brief A Buddy System implementation
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>
#include <datatypes/bitmap.h>

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#define B_TOTAL_EXP 17U
#define B_BLOCK_EXP 6U

#define buddy_left_child(i) (((i) << 1U) + 1U)
#define buddy_right_child(i) (((i) << 1U) + 2U)
#define buddy_parent(i) ((((i) + 1) >> 1U) - 1U)

/// The checkpointable memory context assigned to a single LP
struct mm_state {
	/// The array of checkpoints
	dyn_array(
	    /// Binds a checkpoint together with a reference index
	    struct mm_log {
		    /// The reference index, used to identify this checkpoint
		    array_count_t ref_i;
		    /// A pointer to the actual checkpoint
		    struct mm_checkpoint *c;
	    }) logs;
	/// The count of allocated bytes
	uint_fast32_t used_mem;
	/// The checkpointed binary tree representing the buddy system
	/** the last char is actually unused */
	alignas(16) uint8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The memory buffer served to the model
	alignas(16) unsigned char base_mem[1U << B_TOTAL_EXP];
};
