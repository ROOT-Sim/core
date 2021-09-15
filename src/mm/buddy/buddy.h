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
		}
	) logs;
	/// The count of allocated bytes
	uint_fast32_t used_mem;
	/// The checkpointed binary tree representing the buddy system
	/** the last char is actually unused */
	alignas(16) uint8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The memory buffer served to the model
	alignas(16) unsigned char base_mem[1U << B_TOTAL_EXP];
};

/// A restorable checkpoint of the memory context assigned to a single LP
struct mm_checkpoint { // todo only log longest[] if changed, or incrementally
	/// The used memory in bytes when this checkpoint was taken
	uint_fast32_t used_mem;
	/// The checkpointed binary tree representing the buddy system
	uint8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The checkpointed memory buffer assigned to the model
	unsigned char base_mem[];
};

static_assert(
	offsetof(struct mm_state, longest) ==
	offsetof(struct mm_state, base_mem) -
	sizeof(((struct mm_state *)0)->longest),
	"longest and base_mem are not contiguous, this will break incremental checkpointing");

static_assert(
	offsetof(struct mm_checkpoint, longest) ==
	offsetof(struct mm_checkpoint, base_mem) -
	sizeof(((struct mm_checkpoint *)0)->longest),
	"longest and base_mem are not contiguous, this will break incremental checkpointing");


