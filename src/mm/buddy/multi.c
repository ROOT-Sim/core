/**
 * @file mm/buddy/multi.c
 *
 * @brief Handling of multiple buddy systems
 * TODO: move under mm folder since now this is not anymore buddy specific
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/multi.h>

#include <core/core.h>
#include <core/intrinsics.h>
#include <lp/lp.h>
#include <mm/buddy/buddy.h>
#include <mm/buddy/ckpt.h>
#include <mm/large/large.h>

#include <errno.h>

#ifdef ROOTSIM_INCREMENTAL
#define is_log_incremental(l) ((uintptr_t)(l).c & 0x1)
#else
#define is_log_incremental(l) false
#endif

static inline void buffer_insert(struct mm_state *self, uintptr_t buf)
{
	array_count_t i;
	for(i = 0; i < array_count(self->buffers); ++i)
		if(array_get_at(self->buffers, i) > buf)
			break;

	array_add_at(self->buffers, i, buf);
}

static inline uintptr_t buffer_find_by_address(struct mm_state *self, const void *ptr)
{
	array_count_t l = 0, h = array_count(self->buffers) - 1;
	while(1) {
		array_count_t m = (l + h) / 2;
		uintptr_t b = array_get_at(self->buffers, m) & ~(uintptr_t)1U;
		if(ptr < (void *)b)
			h = m - 1;
		else if(ptr > (void *)(b + sizeof(struct buddy_state)))
			l = m + 1;
		else
			return b;
	}
}

void model_allocator_lp_init(struct mm_state *self)
{
	array_init(self->buffers);
	array_init(self->logs);
	self->full_ckpt_size = offsetof(struct mm_checkpoint, chkps) + sizeof(struct buddy_state *);
}

void model_allocator_lp_fini(struct mm_state *self)
{
	array_count_t i = array_count(self->logs);
	while(i--)
		mm_free(array_get_at(self->logs, i).c);

	array_fini(self->logs);

	i = array_count(self->buffers);
	while(i--)
		mm_free((void *)(array_get_at(self->buffers, i) & ~(uintptr_t)1U));

	array_fini(self->buffers);
}

void *rs_malloc(size_t req_size)
{
	if(unlikely(!req_size))
		return NULL;

	uint_fast8_t req_blks_exp = buddy_allocation_block_compute(req_size);
	struct mm_state *self = &current_lp->mm_state;
	if(unlikely(req_blks_exp > B_TOTAL_EXP)) {
		size_t ts = offsetof(struct large_state, data) + req_size;
		self->full_ckpt_size += ts;
		struct large_state *ls = mm_alloc(ts);
		ls->s = req_size;
		buffer_insert(self, 1U | (uintptr_t)ls);
		return ls->data;
	}

	self->full_ckpt_size += 1 << req_blks_exp;

	array_count_t i = array_count(self->buffers);
	while(i--) {
		uintptr_t buf = array_get_at(self->buffers, i);
		if(unlikely(buf & 1))
			continue;
		void *ret = buddy_malloc((struct buddy_state *)buf, req_blks_exp);
		if(likely(ret != NULL))
			return ret;
	}

	struct buddy_state *new_buddy = mm_alloc(sizeof(*new_buddy));
	buddy_init(new_buddy);
	buffer_insert(self, (uintptr_t)new_buddy);

	self->full_ckpt_size += offsetof(struct buddy_checkpoint, base_mem);
	return buddy_malloc(new_buddy, req_blks_exp);
}

void *rs_calloc(size_t nmemb, size_t size)
{
	size_t tot = nmemb * size;
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
	uintptr_t b = buffer_find_by_address(self, ptr);
	if(unlikely(b & 1U)) {
		struct large_state *ls = (struct large_state *)(b - 1);
		self->full_ckpt_size -= ls->s + offsetof(struct large_state, data);
		ls->s = 0;
	} else {
		self->full_ckpt_size -= buddy_free((struct buddy_state *)b, ptr);
	}
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
	uintptr_t b = buffer_find_by_address(self, ptr);
	if(unlikely(b & 1U)) {
		struct large_state *ls = (struct large_state *)(b - 1);

		if(req_size > ls->s) {
			//TODO
		}
		return ls->data;
	}

	struct buddy_realloc_res ret = buddy_best_effort_realloc((struct buddy_state *)b, ptr, req_size);
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

// todo: incremental
void model_allocator_checkpoint_take(struct mm_state *self, array_count_t ref_i)
{
	struct mm_checkpoint *ckp = mm_alloc(self->full_ckpt_size);
	ckp->ckpt_size = self->full_ckpt_size;

	struct mm_log mm_log = {.ref_i = ref_i, .c = ckp};
	array_push(self->logs, mm_log);

	void *ckpts = ckp->chkps;
	array_count_t i = array_count(self->buffers);
	while(i--) {
		uintptr_t buf = array_get_at(self->buffers, i);
		if(unlikely(buf & 1U))
			ckpts = large_checkpoint_full_take((const struct large_state *)(buf - 1), ckpts);
		else
			ckpts = buddy_checkpoint_full_take((const struct buddy_state *)buf, ckpts);
	}
	((struct buddy_checkpoint *)ckpts)->orig = NULL;
}

void model_allocator_checkpoint_next_force_full(struct mm_state *self)
{
	(void)self;
	// TODO: force full checkpointing when incremental state saving is enabled
}

array_count_t model_allocator_checkpoint_restore(struct mm_state *self, array_count_t ref_i)
{
	array_count_t i = array_count(self->logs) - 1;
	while(array_get_at(self->logs, i).ref_i > ref_i)
		i--;

	struct mm_checkpoint *ckp = array_get_at(self->logs, i).c;
	self->full_ckpt_size = ckp->ckpt_size;
	const void *ckpts = ckp->chkps;

	array_count_t k = array_count(self->buffers);
	while(k--) {
		uintptr_t buf = array_get_at(self->buffers, k);
		const void *c;
		if(unlikely(buf & 1U))
			c = large_checkpoint_full_restore((struct large_state *)(buf - 1), ckpts);
		else
			c = buddy_checkpoint_full_restore((struct buddy_state *)buf, ckpts);

		if(unlikely(c == NULL)) {
			mm_free((void *)(buf & ~(uintptr_t)1U));
			array_remove_at(self->buffers, k);
		} else {
			ckpts = c;
		}
	}

	for(array_count_t j = array_count(self->logs) - 1; j > i; --j)
		mm_free(array_get_at(self->logs, j).c);

	array_count(self->logs) = i + 1;
	return array_get_at(self->logs, i).ref_i;
}

array_count_t model_allocator_fossil_lp_collect(struct mm_state *self, array_count_t tgt_ref_i)
{
	array_count_t log_i = array_count(self->logs) - 1;
	array_count_t ref_i = array_get_at(self->logs, log_i).ref_i;
	while(ref_i > tgt_ref_i) {
		--log_i;
		ref_i = array_get_at(self->logs, log_i).ref_i;
	}

	while(is_log_incremental(array_get_at(self->logs, log_i))) {
		--log_i;
		ref_i = array_get_at(self->logs, log_i).ref_i;
	}

	array_count_t j = array_count(self->logs);
	while(j > log_i) {
		--j;
		array_get_at(self->logs, j).ref_i -= ref_i;
	}

	while(j--)
		mm_free(array_get_at(self->logs, j).c);

	array_truncate_first(self->logs, log_i);
	return ref_i;
}
