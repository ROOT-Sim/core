/**
 * @file mm/checkpoint/full.c
 *
 * @brief Full checkpointing routines
 *
 * This unit contains the implementation of full checkpointing routines for the LP memory management.
 * The model_allocator_checkpoint_take() and model_allocator_checkpoint_restore() entry points
 * dispatch to full or incremental logic based on the runtime configuration and the force_full flag.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/array.h>
#include <mm/buddy/buddy.h>
#include <mm/buddy/checkpoint.h>
#include <mm/checkpoint/checkpoint.h>
#include <mm/model_allocator.h>

extern void model_allocator_checkpoint_take_incremental(struct mm_state *self, array_count_t ref_idx);

/**
 * @brief Takes a full checkpoint of the memory management state.
 *
 * Allocates a checkpoint buffer sized to hold the complete state of all buddy systems,
 * saves each buddy's state, and resets dirty bitmaps when incremental mode is active.
 *
 * @param self    A pointer to the `mm_state` structure.
 * @param ref_idx The reference index (PES position) for this checkpoint.
 */
static void checkpoint_take_full(struct mm_state *self, array_count_t ref_idx)
{
	struct mm_checkpoint *ckpt = mm_alloc(self->full_ckpt_size);
	ckpt->ckpt_size = self->full_ckpt_size;

	const struct mm_log mm_log = {.ref_idx = ref_idx, .ckpt = ckpt};
	array_push(self->logs, mm_log);

	struct buddy_checkpoint *buddy_ckp = (struct buddy_checkpoint *)ckpt->chkps;
	array_count_t i = array_count(self->buddies);
	while(i--)
		buddy_ckp = buddy_checkpoint_full_take(array_get_at(self->buddies, i), buddy_ckp);
	buddy_ckp->orig = NULL;

	if(global_config.incremental_ckpt) {
		self->force_full = false;
		self->ckpt_since_last_full = 0;
		i = array_count(self->buddies);
		while(i--)
			buddy_dirty_reset(array_get_at(self->buddies, i));
	}
}

/**
 * @brief Entry point for taking a checkpoint.
 *
 * Dispatches to a full or incremental checkpoint based on the configuration and state:
 *  - If incremental mode is disabled, always takes a full checkpoint.
 *  - If force_full is set, takes a full checkpoint and resets the flag.
 *  - If full_ckpt_period is set and ckpt_since_last_full has reached it, forces a full checkpoint.
 *  - Otherwise, takes an incremental checkpoint.
 *
 * @param self    A pointer to the `mm_state` structure.
 * @param ref_idx The reference index (PES position) for this checkpoint.
 */
void model_allocator_checkpoint_take(struct mm_state *self, array_count_t ref_idx)
{
	if(!global_config.incremental_ckpt || self->force_full) {
		checkpoint_take_full(self, ref_idx);
		return;
	}

	if(global_config.full_ckpt_period && ++self->ckpt_since_last_full >= global_config.full_ckpt_period) {
		self->force_full = true; /* will be cleared inside checkpoint_take_full */
		checkpoint_take_full(self, ref_idx);
		return;
	}

	model_allocator_checkpoint_take_incremental(self, ref_idx);
}

/**
 * @brief Restores the memory state of all buddy systems from a full checkpoint.
 *
 * @param self    A pointer to the `mm_state` structure.
 * @param ckp     Pointer to the full checkpoint data.
 */
static void restore_from_full(struct mm_state *self, const struct mm_checkpoint *ckp)
{
	self->full_ckpt_size = ckp->ckpt_size;
	const struct buddy_checkpoint *buddy_ckpt = (const struct buddy_checkpoint *)ckp->chkps;

	array_count_t k = array_count(self->buddies);
	while(k--) {
		struct buddy_state *b = array_get_at(self->buddies, k);
		const struct buddy_checkpoint *next = buddy_checkpoint_full_restore(b, buddy_ckpt);
		if(unlikely(next == NULL)) {
			buddy_init(b);
			self->full_ckpt_size += offsetof(struct buddy_checkpoint, base_mem);
		} else {
			buddy_ckpt = next;
		}
	}
}

/**
 * @brief Restores the memory management state to a specific checkpoint.
 *
 * Finds the checkpoint at or before `ref_idx`. If it is a full checkpoint, restores
 * directly. If it is an incremental checkpoint, walks backward through the log chain
 * until a full checkpoint is found, restoring dirty blocks along the way.
 *
 * @param self    A pointer to the `mm_state` structure.
 * @param ref_idx The reference index of the checkpoint to restore.
 * @return The reference index of the restored checkpoint.
 */
array_count_t model_allocator_checkpoint_restore(struct mm_state *self, const array_count_t ref_idx)
{
	array_count_t index = array_count(self->logs) - 1;
	while(array_get_at(self->logs, index).ref_idx > ref_idx)
		index--;

	if(!is_log_incremental(array_get_at(self->logs, index))) {
		// Simple case: target is a full checkpoint.
		restore_from_full(self, log_get_ckpt(array_get_at(self->logs, index)));
	} else {
		/*
		 * Incremental restore: forward apply approach.
		 *
		 * 1. Find the full checkpoint at or before the target in the log chain.
		 * 2. Restore the full checkpoint state via restore_from_full().
		 * 3. Apply each incremental log forward from full+1 to target (inclusive),
		 *    restoring only the dirty blocks recorded in each incremental log.
		 *
		 * This is correct because each incremental log records the state of dirty
		 * blocks AT the time of that checkpoint. Applying them in forward order
		 * produces the state at the target checkpoint. Blocks that were never dirtied
		 * between the full checkpoint and the target come from the full checkpoint.
		 */

		// Find full checkpoint at bottom of chain.
		array_count_t full_i = index;
		while(full_i > 0 && is_log_incremental(array_get_at(self->logs, full_i)))
			full_i--;

		// Restore baseline from full checkpoint.
		restore_from_full(self, log_get_ckpt(array_get_at(self->logs, full_i)));

		// Apply incremental logs forward to target.
		for(array_count_t chain_i = full_i + 1; chain_i <= index; chain_i++) {
			const struct mm_checkpoint *cur_ckp = log_get_ckpt(array_get_at(self->logs, chain_i));
			const struct buddy_checkpoint *bckp = (const struct buddy_checkpoint *)cur_ckp->chkps;

			// Apply each buddy's incremental patch in this log.
			while(bckp->orig != NULL) {
				const struct buddy_checkpoint *next =
				    buddy_checkpoint_incremental_restore((struct buddy_state *)bckp->orig, bckp);
				if(unlikely(next == NULL)) {
					// This buddy is no longer in our state — skip using size.
					const uint_fast32_t dirty_count =
					    bitmap_count_set(bckp->dirty, sizeof(bckp->dirty));
					bckp = (const struct buddy_checkpoint *)((const unsigned char *)bckp->longest +
										 ((size_t)dirty_count << B_BLOCK_EXP));
				} else {
					bckp = next;
				}
			}
		}

		/*
		 * After forward application, restore full_ckpt_size from the target
		 * checkpoint. Each checkpoint (full and incremental) stores the correct
		 * full_ckpt_size at that point in time, reflecting all live allocations.
		 * The restore_from_full() call above only set full_ckpt_size to the full
		 * checkpoint's value; re-applying allocs/frees via incremental patches
		 * changes the buddy trees but NOT full_ckpt_size — so we update it here.
		 */
		self->full_ckpt_size = log_get_ckpt(array_get_at(self->logs, index))->ckpt_size;
	}

	for(array_count_t j = array_count(self->logs) - 1; j > index; --j)
		mm_free(log_get_ckpt(array_get_at(self->logs, j)));

	array_count(self->logs) = index + 1;

	if(global_config.incremental_ckpt) {
		// Reset dirty bitmaps: the state is now clean at the restored checkpoint.
		array_count_t i = array_count(self->buddies);
		while(i--)
			buddy_dirty_reset(array_get_at(self->buddies, i));
		self->ckpt_since_last_full = 0;
		self->last_dirty_buddy = NULL;
	}

	return array_get_at(self->logs, index).ref_idx;
}
