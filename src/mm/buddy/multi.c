/**
 * @file mm/buddy/multi.c
 *
 * @brief Handling of multiple buddy systems
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
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
#define is_log_incremental(l) false
#endif

void model_allocator_lp_init(struct mm_state *self)
{
	array_init(self->buddies);
	array_init(self->logs);
	self->full_ckpt_size = offsetof(struct mm_checkpoint, chkps) + sizeof(struct buddy_state *);
}

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

// todo: incremental
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

void model_allocator_checkpoint_next_force_full(const struct mm_state *self)
{
	(void)self;
	// TODO: force full checkpointing when incremental state saving is enabled
}

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
