/**
 * @file mm/buddy/multi.c
 *
 * @brief Handling of multiple buddy systems
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/multi.h>

#include <arch/timer.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/buddy/ckpt.h>

#include <errno.h>

void model_allocator_lp_init(struct mm_state *self)
{
	self->buddies = NULL;
	array_init(self->logs);
	self->full_ckpt_size = offsetof(struct mm_checkpoint, checkpoints) + sizeof(struct buddy_state *);
}

void model_allocator_lp_fini(struct mm_state *self)
{
	array_count_t i = array_count(self->logs);
	while(i--)
		mm_free(array_get_at(self->logs, i).c);

	array_fini(self->logs);

	for(struct mm_buddy_list *l = self->buddies; l != NULL;) {
		buddy_fini(&l->buddy);

		struct mm_buddy_list *tmp = l;
		l = l->next;
		mm_free(tmp);
	}
}

void *rs_malloc(size_t req_size)
{
	if(unlikely(!req_size))
		return NULL;

	uint_fast8_t req_blks_exp = buddy_allocation_block_compute(req_size);
	if(unlikely(req_blks_exp > B_TOTAL_EXP)) {
		errno = ENOMEM;
		logger(LOG_WARN, "LP %p requested a memory block bigger than %u!", current_lp, 1U << B_TOTAL_EXP);
		return NULL;
	}

	struct mm_state *self = &current_lp->mm_state;
	self->full_ckpt_size += 1 << req_blks_exp;

	for(struct mm_buddy_list *l = self->buddies; l != NULL; l = l->next) {
		void *ret = buddy_malloc(&l->buddy, req_blks_exp);
		if(likely(ret != NULL))
			return ret;
	}

	self->full_ckpt_size += offsetof(struct buddy_checkpoint, base_mem);

	struct mm_buddy_list *new_buddy = mm_alloc(sizeof(*new_buddy));
	buddy_init(&new_buddy->buddy);

	new_buddy->next = self->buddies;
	self->buddies = new_buddy;

	return buddy_malloc(&new_buddy->buddy, req_blks_exp);
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
	self->full_ckpt_size -= buddy_free(ptr);
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
	struct buddy_realloc_res ret = buddy_best_effort_realloc(ptr, req_size);
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
	timer_uint t = timer_hr_new();

	struct mm_checkpoint *ckp = mm_alloc(self->full_ckpt_size);
	ckp->checkpoints_size = self->full_ckpt_size;

	struct mm_log mm_log = {.ref_i = ref_i, .c = ckp};
	array_push(self->logs, mm_log);

	struct buddy_checkpoint *buddy_ckp = (struct buddy_checkpoint *)ckp->checkpoints;
	for(struct mm_buddy_list *l = self->buddies; l != NULL; l = l->next)
		buddy_ckp = checkpoint_full_take(&l->buddy, buddy_ckp);
	buddy_ckp->buddy.chunk = NULL;

	stats_take(STATS_CKPT_SIZE, self->full_ckpt_size);
	stats_take(STATS_CKPT, 1);
	stats_take(STATS_CKPT_TIME, timer_hr_value(t));
}

array_count_t model_allocator_checkpoint_restore(struct mm_state *self, array_count_t ref_i)
{
	array_count_t i = array_count(self->logs) - 1;
	while(array_get_at(self->logs, i).ref_i > ref_i)
		i--;

	struct mm_checkpoint *ckp = array_get_at(self->logs, i).c;
	self->full_ckpt_size = ckp->checkpoints_size;
	const struct buddy_checkpoint *buddy_ckp = (struct buddy_checkpoint *)ckp->checkpoints;

	for(struct mm_buddy_list *l = self->buddies; l != NULL; l = l->next) {
		const struct buddy_checkpoint *c = checkpoint_full_restore(&l->buddy, buddy_ckp);
		if(unlikely(c == buddy_ckp)) {
			buddy_init(&l->buddy);
			self->full_ckpt_size += offsetof(struct buddy_checkpoint, base_mem);
		}
		buddy_ckp = c;
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
