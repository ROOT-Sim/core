/**
 * @file mm/checkpoint/incremental.c
 *
 * @brief Incremental checkpointing routines
 *
 * This unit contains the implementation of incremental checkpointing routines for the LP memory management.
 * Incremental checkpointing saves only the memory blocks that have been dirtied since the last checkpoint,
 * using the dirty bitmap maintained in each buddy system.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/array.h>
#include <lp/lp.h>
#include <mm/buddy/buddy.h>
#include <mm/buddy/checkpoint.h>
#include <mm/checkpoint/checkpoint.h>
#include <mm/model_allocator.h>

/**
 * @brief Forces the next checkpoint to be a full checkpoint.
 *
 * Sets the force_full flag on the mm_state so that the next call to
 * model_allocator_checkpoint_take() will take a full checkpoint regardless
 * of whether incremental mode is enabled. The flag is reset by the full
 * checkpoint routine after it completes.
 *
 * @param self A pointer to the `mm_state` structure representing the memory
 *             management state of the logical process.
 */
void model_allocator_checkpoint_next_force_full(const struct mm_state *self)
{
	((struct mm_state *)self)->force_full = true;
}


/**
 * @brief Marks a memory region as dirty for incremental checkpointing.
 *
 * This function is injected at compile time by the software instrumentation
 * tool before every memory-write instruction in the model code. It marks the
 * corresponding blocks in the buddy system's dirty bitmap, so that only the
 * dirtied blocks are saved in the next incremental checkpoint.
 *
 * LP-visible memory lives in the buddy system's base_mem[] buffer; writes to
 * the longest[] allocation tree are tracked separately by buddy_malloc() and
 * buddy_free() via direct bitmap_set() calls. Therefore, this function only
 * needs to handle writes whose addresses fall within base_mem[].
 *
 * @param ptr  A pointer to the start of the memory region being written to.
 * @param size The size of the memory region being written to, in bytes.
 */
void __write_mem(const void *ptr, const size_t size)
{
	if(unlikely(!global_config.incremental_ckpt))
		return;

	struct mm_state *self = &current_lp->mm_state;
	if(unlikely(!size || array_is_empty(self->buddies)))
		return;

	/*
	 * Fast path: check cached buddy first. This avoids the binary search in
	 * buddy_find_by_address() for consecutive writes to the same buddy, which
	 * is the common case (e.g., repeated rng_state writes within one LP event).
	 */
	struct buddy_state *buddy = self->last_dirty_buddy;
	if(likely(buddy != NULL && ptr >= (void *)buddy && ptr < (void *)(buddy + 1))) {
		buddy_dirty_mark(buddy, ptr, size);
		return;
	}

	/*
	 * Preliminary bounds check: is ptr within any buddy at all? Maybe this could be unneeded if
	 * we relax the requirements in the contract, but I cannot let this check go.
	 */
	if(unlikely(ptr < (void *)array_get_at(self->buddies, 0) || ptr >= (void *)(array_peek(self->buddies) + 1)))
		return;

	buddy = buddy_find_by_address(self, ptr);
	self->last_dirty_buddy = buddy;
	buddy_dirty_mark(buddy, ptr, size);
}

/**
 * @brief Takes an incremental checkpoint of the memory management state.
 *
 * Computes the total size needed to store only the dirty blocks across all buddy
 * systems. Allocates one contiguous mm_checkpoint buffer, fills it by calling
 * buddy_checkpoint_incremental_take() for each buddy, tags the pointer as
 * incremental (bit 0 set), and pushes it onto the log.
 *
 * @param self    A pointer to the mm_state structure.
 * @param ref_idx The reference index (PES position) for this checkpoint.
 */
void model_allocator_checkpoint_take_incremental(struct mm_state *self, array_count_t ref_idx)
{
	// Compute total size needed.
	size_t total = offsetof(struct mm_checkpoint, chkps);
	array_count_t n = array_count(self->buddies);
	for(array_count_t i = 0; i < n; i++)
		total += buddy_checkpoint_incremental_size(array_get_at(self->buddies, i));

	// Sentinel buddy_checkpoint with orig == NULL.
	total += offsetof(struct buddy_checkpoint, longest);

	struct mm_checkpoint *ckpt = mm_alloc(total);
	ckpt->ckpt_size = self->full_ckpt_size;
	ckpt->incr_ckpt_size = (uint_fast32_t)total;

	struct buddy_checkpoint *buddy_ckp = (struct buddy_checkpoint *)ckpt->chkps;
	for(array_count_t i = n; i--;)
		buddy_ckp = buddy_checkpoint_incremental_take(array_get_at(self->buddies, i), buddy_ckp);
	buddy_ckp->orig = NULL; // sentinel

	// Tag as incremental and push to log
	const struct mm_log entry = {.ref_idx = ref_idx, .ckpt = log_mark_incremental(ckpt)};
	array_push(self->logs, entry);
}
