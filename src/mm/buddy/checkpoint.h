/**
 * @file mm/buddy/checkpoint.h
 *
 * @brief Buddy system checkpointing capabilities
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <mm/buddy/buddy.h>

/// A restorable checkpoint of the memory context of a single buddy system.
/// For a full checkpoint, both `longest[]` and `base_mem[]` are populated.
/// For an incremental checkpoint, only dirty blocks (as tracked by `dirty[]`) are stored
/// in packed form starting at `longest[]`.
struct buddy_checkpoint {
	/// The buddy system to which this checkpoint applies
	const struct buddy_state *orig;
	/// The checkpoint of the dirty bitmap
	block_bitmap dirty[bitmap_required_size(
	    // this tracks writes to the allocation tree...
	    (1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1)) +
	    // ...while this tracks writes to the actual memory buffer
	    (1 << (B_TOTAL_EXP - B_BLOCK_EXP)))];
	/// The checkpointed binary tree representing the buddy system
	uint8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The checkpointed memory buffer assigned to the model
	unsigned char base_mem[];
};

static_assert(offsetof(struct buddy_checkpoint, longest) ==
		  offsetof(struct buddy_checkpoint, base_mem) - sizeof(((struct buddy_checkpoint *)0)->longest),
    "longest and base_mem are not contiguous, this will break incremental checkpointing");

extern struct buddy_checkpoint *buddy_checkpoint_full_take(const struct buddy_state *self, struct buddy_checkpoint *data);
extern const struct buddy_checkpoint *buddy_checkpoint_full_restore(struct buddy_state *self,
    const struct buddy_checkpoint *data);

extern struct buddy_checkpoint *buddy_checkpoint_incremental_take(const struct buddy_state *self,
    struct buddy_checkpoint *data);
extern const struct buddy_checkpoint *buddy_checkpoint_incremental_restore(struct buddy_state *self,
    const struct buddy_checkpoint *ckp);

/// Restores dirty blocks from an incremental checkpoint for blocks still set in `remaining`.
/// Clears restored bits from `remaining`. Returns pointer past consumed data, or NULL if no match.
extern const struct buddy_checkpoint *buddy_checkpoint_incremental_restore_partial(struct buddy_state *self,
    const struct buddy_checkpoint *ckp, block_bitmap *remaining);

/// Restores blocks still set in `remaining` from a full checkpoint.
/// Returns pointer past consumed data, or NULL if no match.
extern const struct buddy_checkpoint *buddy_checkpoint_full_restore_remaining(struct buddy_state *self,
    const struct buddy_checkpoint *ckp, const block_bitmap *remaining);

/// Computes the size in bytes needed for an incremental checkpoint of this buddy system.
extern size_t buddy_checkpoint_incremental_size(const struct buddy_state *self);
