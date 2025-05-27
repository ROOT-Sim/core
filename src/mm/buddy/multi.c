/**
 * @file mm/buddy/multi.c
 *
 * @brief Handling of multiple buddy systems
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/multi.h>

#include <core/core.h>
#include <core/intrinsics.h>
#include <lp/lp.h>
#include <mm/buddy/buddy.h>
#include <mm/buddy/ckpt.h>

#include <errno.h>

#ifdef ROOTSIM_INCREMENTAL
/// Tells whether a checkpoint is incremental or not.
#define is_log_incremental(l) ((uintptr_t)(l).c & 0x1)
#else
/// Tells whether a checkpoint is incremental or not.
#define is_log_incremental(l) false
#endif

/**
 * @brief Initializes the memory management state for a logical process.
 *
 * This function sets up the memory management state for a logical process by
 * initializing the arrays used to manage buddy systems and logs. It also
 * calculates the initial size of a full checkpoint based on the structure
 * layout.
 *
 * @param self A pointer to the `mm_state` structure representing the memory
 *             management state of the logical process.
 */
void model_allocator_lp_init(struct mm_state *self)
{
	array_init(self->buddies);
	array_init(self->logs);
	self->full_ckpt_size = offsetof(struct mm_checkpoint, chkps) + sizeof(struct buddy_state *);
}


/**
 * @brief Finalizes the memory management state for a logical process.
 *
 * This function releases all resources associated with the memory management
 * state of a logical process. It frees all memory allocated for logs and buddy
 * systems and ensures proper cleanup of the associated arrays.
 *
 * @param self A pointer to the `mm_state` structure representing the memory
 *             management state of the logical process.
 */
void model_allocator_lp_fini(const struct mm_state *self)
{
	array_count_t index = array_count(self->logs);
	while(index--)
		mm_free(array_get_at(self->logs, index).ckpt);

	array_fini(self->logs);

	index = array_count(self->buddies);
	while(index--)
		mm_free(array_get_at(self->buddies, index));

	array_fini(self->buddies);
}


void *rs_malloc(size_t req_size)
{
	if(unlikely(!req_size))
		return NULL;

	const uint_fast8_t req_blks_exp = buddy_allocation_block_compute(req_size);
	if(unlikely(req_blks_exp > B_TOTAL_EXP)) {
		errno = ENOMEM;
		logger(LOG_WARN, "LP %p requested a memory block bigger than %u!", current_lp, 1U << B_TOTAL_EXP);
		return NULL;
	}

	struct mm_state *self = &current_lp->mm_state;
	self->full_ckpt_size += 1 << req_blks_exp;

	array_count_t index = array_count(self->buddies);
	while(index--) {
		void *ret = buddy_malloc(array_get_at(self->buddies, index), req_blks_exp);
		if(likely(ret != NULL))
			return ret;
	}

	struct buddy_state *new_buddy = mm_alloc(sizeof(*new_buddy));
	buddy_init(new_buddy);

	for(index = 0; index < array_count(self->buddies); ++index)
		if(array_get_at(self->buddies, index) > new_buddy)
			break;

	array_add_at(self->buddies, index, new_buddy);
	self->full_ckpt_size += offsetof(struct buddy_checkpoint, base_mem);
	return buddy_malloc(new_buddy, req_blks_exp);
}

void *rs_calloc(const size_t nmemb, const size_t size)
{
	const size_t tot = nmemb * size;
	void *ret = rs_malloc(tot);

	if(likely(ret))
		memset(ret, 0, tot);

	return ret;
}

/**
 * @brief Finds the buddy system managing a given memory address.
 *
 * This function performs a binary search to locate the `buddy_state` structure
 * that manages the memory block containing the specified address.
 *
 * @param self A pointer to the `mm_state` structure representing the memory
 *             management state of the logical process.
 * @param ptr A pointer to the memory address to locate within the buddy system.
 * @return A pointer to the `buddy_state` structure managing the memory block
 *         containing the specified address.
 */
static inline struct buddy_state *buddy_find_by_address(const struct mm_state *self, const void *ptr)
{
	array_count_t low = 0, high = array_count(self->buddies) - 1;
	while(1) {
		const array_count_t middle = (low + high) / 2;
		struct buddy_state *buddy = array_get_at(self->buddies, middle);
		if(ptr < (void *)buddy)
			high = middle - 1;
		else if(ptr > (void *)(buddy + 1))
			low = middle + 1;
		else
			return buddy;
	}
}


void rs_free(void *ptr)
{
	if(unlikely(!ptr))
		return;

	struct mm_state *self = &current_lp->mm_state;
	struct buddy_state *buddy = buddy_find_by_address(self, ptr);
	self->full_ckpt_size -= buddy_free(buddy, ptr);
}


void *rs_realloc(void *ptr, size_t req_size)
{
	if(!req_size) { // Adhering to C11 standard §7.20.3.1
		if(!ptr)
			errno = EINVAL;
		return NULL;
	}
	if(!ptr)
		return rs_malloc(req_size);

	struct mm_state *self = &current_lp->mm_state;
	struct buddy_state *buddy = buddy_find_by_address(self, ptr);
	struct buddy_realloc_res ret = buddy_best_effort_realloc(buddy, ptr, req_size);
	if(ret.handled) {
		self->full_ckpt_size += ret.variation;
		return ptr;
	}

	void *new_buffer = rs_malloc(req_size);
	if(unlikely(new_buffer == NULL))
		return NULL;

	memcpy(new_buffer, ptr, min(req_size, ret.original));
	rs_free(ptr);

	return new_buffer;
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
		buddy_ckp = checkpoint_full_take(array_get_at(self->buddies, i), buddy_ckp);
	buddy_ckp->orig = NULL;
}

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
		const struct buddy_checkpoint *ckpt = checkpoint_full_restore(array_get_at(self->buddies, k), buddy_ckpt);
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


/**
 * @brief Collects fossil logs up to a target reference index.
 *
 * This function removes logs from the memory management state that are no longer
 * needed, based on the specified target reference index. It ensures that only
 * the logs required for restoring the state up to the target index are retained.
 *
 * @param self A pointer to the `mm_state` structure representing the memory
 *             management state of the logical process.
 * @param tgt_ref_i The target reference index up to which logs should be collected.
 * @return The reference index of the last retained log.
 */
array_count_t model_allocator_fossil_lp_collect(struct mm_state *self, const array_count_t tgt_ref_i)
{
	array_count_t log_i = array_count(self->logs) - 1;
	array_count_t ref_i = array_get_at(self->logs, log_i).ref_idx;
	while(ref_i > tgt_ref_i) {
		--log_i;
		ref_i = array_get_at(self->logs, log_i).ref_idx;
	}

	while(is_log_incremental(array_get_at(self->logs, log_i))) {
		--log_i;
		ref_i = array_get_at(self->logs, log_i).ref_idx;
	}

	array_count_t j = array_count(self->logs);
	while(j > log_i) {
		--j;
		array_get_at(self->logs, j).ref_idx -= ref_i;
	}

	while(j--)
		mm_free(array_get_at(self->logs, j).ckpt);

	array_truncate_first(self->logs, log_i);
	return ref_i;
}
