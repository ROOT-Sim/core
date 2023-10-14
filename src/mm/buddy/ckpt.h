/**
 * @file mm/buddy/ckpt.h
 *
 * @brief Checkpointing capabilities
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <mm/buddy/buddy.h>

/// A restorable checkpoint of the memory context of a single buddy system
struct buddy_checkpoint {
	struct buddy_state buddy;
	/// The checkpointed memory buffer assigned to the model
	unsigned char base_mem[];
};

extern struct buddy_checkpoint *checkpoint_full_take(const struct buddy_state *self, struct buddy_checkpoint *data);
extern const struct buddy_checkpoint *checkpoint_full_restore(struct buddy_state *self, const struct buddy_checkpoint *data);
extern struct buddy_checkpoint *checkpoint_incremental_take(const struct buddy_state *self, struct buddy_checkpoint *data);
extern const struct buddy_checkpoint * checkpoint_incremental_restore(struct buddy_state *self, const struct buddy_checkpoint *ckp);
