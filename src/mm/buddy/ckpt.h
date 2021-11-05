#pragma once

#include <mm/buddy/buddy.h>

/// A restorable checkpoint of the memory context assigned to a single LP
struct mm_checkpoint { // todo only log longest[] if changed, or incrementally
	/// The size in bytes of this checkpoint
	uint_fast32_t ckpt_size;
	/// The used memory in bytes when this checkpoint was taken
	uint_fast32_t used_mem;
	/// The checkpointed binary tree representing the buddy system
	uint8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The checkpointed memory buffer assigned to the model
	unsigned char base_mem[];
};

extern struct mm_checkpoint *checkpoint_full_take(const struct mm_state *self);
extern void checkpoint_full_free(struct mm_checkpoint *ckp);
extern void checkpoint_full_restore(struct mm_state *self,
		const struct mm_checkpoint *ckp);
