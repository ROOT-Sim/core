/**
 * @file mm/buddy/multi.c
 *
 * @brief Handling of multiple buddy systems
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/multi_buddy.h>

#include <core/core.h>
#include <core/intrinsics.h>
#include <lp/lp.h>
#include <mm/buddy/buddy.h>
#include <mm/buddy/ckpt.h>

#include <errno.h>

#ifdef ROOTSIM_INCREMENTAL
#define is_log_incremental(l) ((uintptr_t)(l).c & 0x1)
#else
#define is_log_incremental(l) false
#endif

void multi_buddy_lp_init(struct multi_buddy_state *self)
{
	array_init(self->buddies);
	array_init(self->logs);
	self->used_mem = 0;
}

void multi_buddy_lp_fini(struct multi_buddy_state *self)
{
	array_count_t i = array_count(self->logs);
	while(i--)
		mm_free(array_get_at(self->logs, i).c);

	array_fini(self->logs);

	i = array_count(self->buddies);
	while(i--)
		mm_free(array_get_at(self->buddies, i));

	array_fini(self->buddies);
}

void *multi_buddy_alloc(struct multi_buddy_state *self, size_t req_size)
{
	uint_fast8_t req_blks_exp = buddy_allocation_block_compute(req_size);
	if(unlikely(req_blks_exp > B_TOTAL_EXP)) {
		errno = ENOMEM;
		logger(LOG_WARN, "LP %p requested a memory block bigger than %u!", current_lp, 1U << B_TOTAL_EXP);
		return NULL;
	}

	self->used_mem += 1 << req_blks_exp;

	array_count_t i = array_count(self->buddies);
	while(i--) {
		void *ret = buddy_malloc(array_get_at(self->buddies, i), req_blks_exp);
		if(likely(ret != NULL))
			return ret;
	}

	struct buddy_state *new_buddy = mm_alloc(sizeof(*new_buddy));
	buddy_init(new_buddy);

	for(i = 0; i < array_count(self->buddies); ++i)
		if(array_get_at(self->buddies, i) > new_buddy)
			break;

	array_add_at(self->buddies, i, new_buddy);
	return buddy_malloc(new_buddy, req_blks_exp);
}

static inline struct buddy_state *buddy_find_by_address(struct multi_buddy_state *self, const void *ptr)
{
	array_count_t l = 0, h = array_count(self->buddies) - 1;
	while(1) {
		array_count_t m = (l + h) / 2;
		struct buddy_state *b = array_get_at(self->buddies, m);
		if(ptr < (void *)b)
			h = m - 1;
		else if(ptr > (void *)(b + 1))
			l = m + 1;
		else
			return b;
	}
}

void multi_buddy_free(struct multi_buddy_state *self, void *ptr)
{
	struct buddy_state *b = buddy_find_by_address(self, ptr);
	self->used_mem -= buddy_free(b, ptr);
}

void *multi_buddy_realloc(struct multi_buddy_state *self, void *ptr, size_t req_size)
{
	struct buddy_state *b = buddy_find_by_address(self, ptr);
	struct buddy_realloc_res ret = buddy_best_effort_realloc(b, ptr, req_size);
	if (ret.handled) {
		self->used_mem += ret.variation;
		return ptr;
	}

	void *new_buffer = rs_malloc(req_size);
	if(unlikely(new_buffer == NULL))
		return NULL;

	memcpy(new_buffer, ptr, min(req_size, ret.original));
	rs_free(ptr);

	return new_buffer;
}

void multi_buddy_dirty_mark(struct multi_buddy_state *self, const void *ptr, size_t s)
{
	if(unlikely(!s || array_is_empty(self->buddies)))
		return;

	if(unlikely(ptr < (void *)array_get_at(self->buddies, 0) || ptr > (void *)(array_peek(self->buddies) + 1)))
		return;

	struct buddy_state *b = buddy_find_by_address(self, ptr);

	buddy_dirty_mark(b, ptr, s);
}

// todo: incremental
void multi_buddy_checkpoint_take(struct multi_buddy_state *self, array_count_t ref_i)
{
	size_t buddies_size = offsetof(struct buddy_checkpoint, base_mem) * array_count(self->buddies);
	struct mm_checkpoint *ckp = mm_alloc(offsetof(struct mm_checkpoint, chkps) + buddies_size + self->used_mem);
	ckp->used_mem = self->used_mem;

	struct mm_log mm_log = {.ref_i = ref_i, .c = ckp};
	array_push(self->logs, mm_log);

	struct buddy_checkpoint *buddy_ckp = (struct buddy_checkpoint *)ckp->chkps;
	array_count_t i = array_count(self->buddies);
	while(i--)
		buddy_ckp = checkpoint_full_take(array_get_at(self->buddies, i), buddy_ckp);
}

void multi_buddy_checkpoint_next_force_full(struct multi_buddy_state *self)
{
	// TODO: force full checkpointing when incremental state saving is enabled
	(void) self;
}

array_count_t multi_buddy_checkpoint_restore(struct multi_buddy_state *self, array_count_t ref_i)
{
	array_count_t i = array_count(self->logs) - 1;
	while(array_get_at(self->logs, i).ref_i > ref_i)
		i--;

	struct mm_checkpoint *ckp = array_get_at(self->logs, i).c;
	self->used_mem = ckp->used_mem;
	const struct buddy_checkpoint *buddy_ckp = (struct buddy_checkpoint *)ckp->chkps;

	array_count_t k = array_count(self->buddies);
	while(k--) {
		struct buddy_state *b = array_get_at(self->buddies, k);
		const struct buddy_checkpoint *c = checkpoint_full_restore(array_get_at(self->buddies, k), buddy_ckp);
		if (unlikely(c == NULL))
			buddy_init(b);
		else
			buddy_ckp = c;
	}

	for(array_count_t j = array_count(self->logs) - 1; j > i; --j)
		mm_free(array_get_at(self->logs, j).c);

	array_count(self->logs) = i + 1;
	return array_get_at(self->logs, i).ref_i;
}

array_count_t multi_buddy_fossil_lp_collect(struct multi_buddy_state *self, array_count_t tgt_ref_i)
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
