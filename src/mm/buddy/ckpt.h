/**
* @file mm/buddy/ckpt.h
*
* @brief Checkpointing capabilities
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <mm/buddy/buddy.h>

/// A restorable checkpoint of the memory context assigned to a single LP
struct mm_checkpoint { // todo only log longest[] if changed, or incrementally
	_Bool is_incremental;
	/// The checkpoint of the dirty bitmap
	block_bitmap dirty [
		bitmap_required_size(
		// this tracks writes to the allocation tree
			(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1)) +
		// while this tracks writes to the actual memory buffer
			(1 << (B_TOTAL_EXP - B_BLOCK_EXP))
		)
	];
	/// The used memory in bytes when this checkpoint was taken
	uint_fast32_t used_mem;
	/// The checkpointed binary tree representing the buddy system
	uint8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The checkpointed memory buffer assigned to the model
	unsigned char base_mem[];
};

static_assert(
	offsetof(struct mm_checkpoint, longest) ==
	offsetof(struct mm_checkpoint, base_mem) -
	sizeof(((struct mm_checkpoint *)0)->longest),
	"longest and base_mem are not contiguous, this will break incremental checkpointing");

#define checkpoint_free(x) mm_free(x)
extern struct mm_checkpoint *checkpoint_full_take(const struct mm_state *self);
extern void checkpoint_full_restore(struct mm_state *self, const struct mm_checkpoint *ckp);
extern struct mm_checkpoint *checkpoint_incremental_take(const struct mm_state *self);
extern void checkpoint_incremental_restore(struct mm_state *self, const struct mm_checkpoint *ckp);
