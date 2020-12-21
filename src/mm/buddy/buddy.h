/**
* @file mm/buddy/buddy.h
*
* @brief A Buddy System implementation
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
#include <datatypes/bitmap.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define B_TOTAL_EXP 17U
#define B_BLOCK_EXP 6U
#define B_LOG_INCREMENTAL_THRESHOLD 0.5
#define B_LOG_FREQUENCY 50

/// The checkpointable memory context assigned to a single LP
struct mm_state { // todo incremental checkpoints
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
	uint_least8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))]; // last char is actually unused
	/// The memory buffer served to the model
	unsigned char base_mem[1U << B_TOTAL_EXP];
#ifdef ROOTSIM_INCREMENTAL
	/// The bytes count of the memory dirtied by writes
	uint_fast32_t dirty_mem;
	/// This keeps track of memory blocks which have been dirtied by a write
	block_bitmap dirty[
		bitmap_required_size(
		// this tracks writes to the allocation tree
			(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1)) +
		// while this tracks writes to the actual memory buffer
			(1 << (B_TOTAL_EXP - B_BLOCK_EXP))

		)
	];
#endif
};

/// A restorable checkpoint of the memory context assigned to a single LP
struct mm_checkpoint { // todo only log longest[] if changed, or incrementally
#ifdef ROOTSIM_INCREMENTAL
	/// If set this checkpoint is incremental, else it is a full one
	bool is_incremental;
	/// The checkpoint of the dirty bitmap
	block_bitmap dirty [
		bitmap_required_size(
		// this tracks writes to the allocation tree
			(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1)) +
		// while this tracks writes to the actual memory buffer
			(1 << (B_TOTAL_EXP - B_BLOCK_EXP))
		)
	];
#endif
	/// The used memory in bytes when this checkpoint was taken
	uint_fast32_t used_mem;
	/// The checkpointed binary tree representing the buddy system
	uint_least8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The checkpointed memory buffer assigned to the model
	unsigned char base_mem[];
};

static_assert(
	offsetof(struct mm_state, longest) ==
	offsetof(struct mm_state, base_mem) -
	sizeof(((struct mm_state *)0)->longest),
	"longest and base_mem are not contiguous, this will break incremental checkpointing");


