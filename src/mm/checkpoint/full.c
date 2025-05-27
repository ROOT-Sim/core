/**
* @file mm/checkpoint/full.c
 *
 * @brief Full checkpointing routines
 *
 * This unit contains the implementation of full checkpointing routines for the LP memory management
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/array.h>
#include <mm/buddy/checkpoint.h>
#include <mm/checkpoint/checkpoint.h>
/**
 * @brief Takes a full checkpoint of the memory management state.
 *
 * This function creates a full checkpoint of the memory management state for a logical process.
 * It allocates memory for the checkpoint, records its size, and stores it in the logs. Each buddy
 * system's state is saved into the checkpoint structure.
 *
 * @param self A pointer to the `mm_state` structure representing the memory management state.
 * @param ref_idx The reference index associated with the checkpoint.
 */
void model_allocator_checkpoint_take(struct mm_state *self, array_count_t ref_idx)
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
}


/**
 * @brief Restores the memory management state to a specific checkpoint.
 *
 * This function restores the memory management state of a logical process to the state
 * recorded in the checkpoint corresponding to the given reference index. It ensures
 * that all buddy systems are restored to their respective states as recorded in the
 * checkpoint. If a checkpoint is missing for a buddy system, it reinitializes the buddy
 * system to a default state.
 *
 * @param self A pointer to the `mm_state` structure representing the memory
 *             management state of the logical process.
 * @param ref_idx The reference index of the checkpoint to restore.
 * @return The reference index of the restored checkpoint.
 */
array_count_t model_allocator_checkpoint_restore(struct mm_state *self, const array_count_t ref_idx)
{
	array_count_t index = array_count(self->logs) - 1;
	while(array_get_at(self->logs, index).ref_idx > ref_idx)
		index--;

	const struct mm_checkpoint *ckp = array_get_at(self->logs, index).ckpt;
	self->full_ckpt_size = ckp->ckpt_size;
	const struct buddy_checkpoint *buddy_ckpt = (struct buddy_checkpoint *)ckp->chkps;

	array_count_t k = array_count(self->buddies);
	while(k--) {
		struct buddy_state *b = array_get_at(self->buddies, k);
		const struct buddy_checkpoint *ckpt =
		    buddy_checkpoint_full_restore(array_get_at(self->buddies, k), buddy_ckpt);
		if(unlikely(ckpt == NULL)) {
			buddy_init(b);
			self->full_ckpt_size += offsetof(struct buddy_checkpoint, base_mem);
		} else {
			buddy_ckpt = ckpt;
		}
	}

	for(array_count_t j = array_count(self->logs) - 1; j > index; --j)
		mm_free(array_get_at(self->logs, j).ckpt);

	array_count(self->logs) = index + 1;
	return array_get_at(self->logs, index).ref_idx;
}
