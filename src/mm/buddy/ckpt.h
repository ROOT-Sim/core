/**
 * @file mm/buddy/ckpt.h
 *
 * @brief Checkpointing capabilities
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <mm/buddy/buddy.h>

/// A restorable checkpoint of the memory context of a single buddy system
struct buddy_checkpoint { // todo only log longest[] if changed, or incrementally
	/// The buddy system to which this checkpoint applies. TODO: reengineer the multi-checkpointing approach
	const struct distr_mem_chunk *orig;
	/// The checkpoint of the dirty bitmap
	block_bitmap dirty [
		bitmap_required_size(
		// this tracks writes to the allocation tree...
			(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1)) +
		// ...while this tracks writes to the actual memory buffer
			(1 << (B_TOTAL_EXP - B_BLOCK_EXP))
		)
	];
	/// The checkpointed binary tree representing the buddy system
	uint8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The checkpointed memory buffer assigned to the model
	unsigned char base_mem[];
};

static_assert(
	offsetof(struct buddy_checkpoint, longest) ==
	offsetof(struct buddy_checkpoint, base_mem) -
	sizeof(((struct buddy_checkpoint *)0)->longest),
	"longest and base_mem are not contiguous, this will break incremental checkpointing");

extern struct buddy_checkpoint *checkpoint_full_take(const struct buddy_state *self, struct buddy_checkpoint *data);
extern const struct buddy_checkpoint *checkpoint_full_restore(struct buddy_state *self, const struct buddy_checkpoint *data);
extern struct buddy_checkpoint *checkpoint_incremental_take(const struct buddy_state *self, struct buddy_checkpoint *data);
extern const struct buddy_checkpoint * checkpoint_incremental_restore(struct buddy_state *self, const struct buddy_checkpoint *ckp);
