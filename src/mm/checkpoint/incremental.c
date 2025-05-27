/**
* @file mm/checkpoint/incremental.c
 *
 * @brief Incremental checkpointing routines
 *
 * This unit contains the implementation of incremental checkpointing routines for the LP memory management
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/array.h>
#include <lp/lp.h>
#include <mm/buddy/buddy.h>
#include <mm/checkpoint/checkpoint.h>

/**
 * @brief Forces the next checkpoint to be a full checkpoint.
 *
 * This function is a placeholder for enabling full checkpointing when
 * incremental state saving is active. Currently, it does nothing as
 * incremental checkpointing is disabled.
 *
 * @param self A pointer to the `mm_state` structure representing the memory
 *             management state of the logical process.
 */
void model_allocator_checkpoint_next_force_full(const struct mm_state *self)
{
	(void)self;
	// TODO: force full checkpointing when incremental state saving is enabled
}


/**
 * @brief Marks a memory region as dirty for incremental checkpointing.
 *
 * This function is intended to be an entry point for the model's code, injected at compile time,
 * to mark a memory region as dirty whenever a write operation is performed. It updates the
 * corresponding buddy system to track the modified memory region.
 *
 * @note This function is currently unused because the incremental checkpointing subsystem
 *       is disabled.
 *
 * @param ptr A pointer to the start of the memory region being written to.
 * @param size The size of the memory region being written to, in bytes.
 */
void __write_mem(const void *ptr, const size_t size)
{
	struct mm_state *self = &current_lp->mm_state;
	if(unlikely(!size || array_is_empty(self->buddies)))
		return;

	if(unlikely(ptr < (void *)array_get_at(self->buddies, 0) || ptr > (void *)(array_peek(self->buddies) + 1)))
		return;

	struct buddy_state *buddy = buddy_find_by_address(self, ptr);

	buddy_dirty_mark(buddy, ptr, size);
}
