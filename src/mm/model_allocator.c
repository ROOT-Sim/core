/**
 * @file mm/model_allocator.c
 *
 * @brief The subsystem managing the memory state of an LP
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <errno.h>
#include <stddef.h>

#include <mm/buddy/buddy.h>
#include <mm/buddy/checkpoint.h>
#include <core/core.h>
#include <log/log.h>
#include <lp/lp.h>


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
struct buddy_state *buddy_find_by_address(const struct mm_state *self, const void *ptr)
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
