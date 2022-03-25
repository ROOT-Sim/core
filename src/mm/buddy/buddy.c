/**
 * @file mm/buddy/buddy.c
 *
 * @brief A Buddy System implementation
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <errno.h>
#include <stdlib.h>

#include <mm/buddy/buddy.h>
#include <core/core.h>
#include <core/intrinsics.h>
#include <lp/lp.h>
#include <mm/buddy/ckpt.h>
#include <ROOT-Sim.h>


#define is_power_of_2(i) (!((i) & ((i)-1)))
#define next_exp_of_2(i) (sizeof(i) * CHAR_BIT - intrinsics_clz(i))

void model_allocator_lp_init(void)
{
	struct mm_state *self = &current_lp->mm_state;
	uint_fast8_t node_size = B_TOTAL_EXP;

	for(uint_fast32_t i = 0; i < sizeof(self->longest) / sizeof(*self->longest); ++i) {
		self->longest[i] = node_size;
		node_size -= is_power_of_2(i + 2);
	}

	self->used_mem = 0;
	array_init(self->logs);
}

void model_allocator_lp_fini(void)
{
	struct mm_state *self = &current_lp->mm_state;
	array_count_t i = array_count(self->logs);
	while(i--)
		checkpoint_free(array_get_at(self->logs, i).c);

	array_fini(self->logs);
}

void *rs_malloc(size_t req_size)
{
	if(unlikely(!req_size))
		return NULL;

	struct mm_state *self = &current_lp->mm_state;
	uint_fast8_t req_blks = max(next_exp_of_2(req_size - 1), B_BLOCK_EXP);

	if(unlikely(self->longest[0] < req_blks)) {
		errno = ENOMEM;
		logger(LOG_WARN, "LP %p is out of memory!", current_lp);
		return NULL;
	}

	/* search recursively for the child */
	uint_fast8_t node_size = B_TOTAL_EXP;
	uint_fast32_t i = 0;
	while(node_size > req_blks) {
		/* choose the child with smaller longest value which
		 * is still large at least *size* */
		i = buddy_left_child(i);
		i += self->longest[i] < req_blks;
		--node_size;
	}

	/* update the *longest* value back */
	self->longest[i] = 0;
	self->used_mem += 1 << node_size;
#ifdef ROOTSIM_INCREMENTAL
	bitmap_set(self->dirty, i >> B_BLOCK_EXP);
#endif

	uint_fast32_t offset = ((i + 1) << node_size) - (1 << B_TOTAL_EXP);

	while(i) {
		i = buddy_parent(i);
		self->longest[i] = max(self->longest[buddy_left_child(i)], self->longest[buddy_right_child(i)]);
#ifdef ROOTSIM_INCREMENTAL
		bitmap_set(self->dirty, i >> B_BLOCK_EXP);
#endif

	}

	return ((char *)self->base_mem) + offset;
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
	uint_fast8_t node_size = B_BLOCK_EXP;
	uint_fast32_t o = ((uintptr_t)ptr - (uintptr_t)self->base_mem) >> B_BLOCK_EXP;
	uint_fast32_t i = o + (1 << (B_TOTAL_EXP - B_BLOCK_EXP)) - 1;

	for(; self->longest[i]; i = buddy_parent(i))
		++node_size;

	self->longest[i] = node_size;
	self->used_mem -= 1 << node_size;
#ifdef ROOTSIM_INCREMENTAL
	bitmap_set(self->dirty, i >> B_BLOCK_EXP);

	uint_fast32_t b = (1 << (node_size - B_BLOCK_EXP)) - 1;
	o += (1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));
	// need to track freed blocks content because full checkpoints don't
	do {
		bitmap_set(self->dirty, o + b);
	} while(b--);

#endif

	while(i) {
		i = buddy_parent(i);

		uint_fast8_t left_long = self->longest[buddy_left_child(i)];
		uint_fast8_t right_long = self->longest[buddy_right_child(i)];

		if(left_long == node_size && right_long == node_size) {
			self->longest[i] = node_size + 1;
		} else {
			self->longest[i] = max(left_long, right_long);
		}
#ifdef ROOTSIM_INCREMENTAL
		bitmap_set(self->dirty, i >> B_BLOCK_EXP);
#endif
		++node_size;
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
	uint_fast8_t node_size = B_BLOCK_EXP;
	uint_fast32_t o = ((uintptr_t)ptr - (uintptr_t)self->base_mem) >> B_BLOCK_EXP;
	uint_fast32_t i = o + (1 << (B_TOTAL_EXP - B_BLOCK_EXP)) - 1;

	for(; self->longest[i]; i = buddy_parent(i))
		++node_size;

	size_t old_size = 1 << node_size;
	size_t new_size = 1 << max(next_exp_of_2(req_size - 1), B_BLOCK_EXP);

	if(old_size == new_size)
		return ptr;

	void *new_buffer = rs_malloc(req_size);
	if(unlikely(new_buffer == NULL)) {
		errno = ENOMEM;
		return NULL;
	}

	memcpy(new_buffer, ptr, min(new_size, old_size));
	rs_free(ptr);

	return new_buffer;
}

void __write_mem(const void *ptr, size_t siz)
{
	if (!siz)
		return;

	struct mm_state *self = &current_lp->mm_state;
	uintptr_t diff = (uintptr_t)ptr - (uintptr_t)self->base_mem;
	if (diff >= (1 << B_TOTAL_EXP))
		return;

	uint_fast32_t i = (diff >> B_BLOCK_EXP) +
		(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));

	self->dirty_mem += siz;

	siz += diff & ((1 << B_BLOCK_EXP) - 1);
	--siz;
	siz >>= B_BLOCK_EXP;

	do {
		bitmap_set(self->dirty, i + siz);
	} while(siz--);
}

void model_allocator_checkpoint_take(array_count_t ref_i)
{
	struct mm_state *self = &current_lp->mm_state;
	struct mm_checkpoint *ckp;

#ifdef ROOTSIM_INCREMENTAL
	if (self->dirty_mem < self->used_mem * B_LOG_INCREMENTAL_THRESHOLD) {
		ckp = checkpoint_incremental_take(self);
	} else {
#endif
		ckp = checkpoint_full_take(self);
#ifdef ROOTSIM_INCREMENTAL
	}
	memset(self->dirty, 0, sizeof(self->dirty));
#endif

	struct mm_log mm_log = {
		.ref_i = ref_i,
		.c = ckp
	};
	array_push(self->logs, mm_log);
}

void model_allocator_checkpoint_next_force_full(void)
{
#ifdef ROOTSIM_INCREMENTAL
	struct mm_state *self = &current_lp->mm_state;
	self->dirty_mem = UINT32_MAX;
#endif
}

array_count_t model_allocator_checkpoint_restore(array_count_t ref_i)
{
	struct mm_state *self = &current_lp->mm_state;
	array_count_t i = array_count(self->logs) - 1;
	while(array_get_at(self->logs, i).ref_i > ref_i)
		i--;

	const struct mm_checkpoint *ckp = array_get_at(self->logs, i).c;
#ifdef ROOTSIM_INCREMENTAL
	if (ckp->is_incremental) {
		checkpoint_incremental_restore(self, ckp);
	} else {
#endif
		checkpoint_full_restore(self, ckp);
#ifdef ROOTSIM_INCREMENTAL
	}
	memset(self->dirty, 0, sizeof(self->dirty));
	self->dirty_mem = 0;
#endif

	for(array_count_t j = array_count(self->logs) - 1; j > i; --j)
		checkpoint_free(array_get_at(self->logs, j).c);

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

	while (array_get_at(self->logs, log_i).c->is_incremental) {
		--log_i;
		ref_i = array_get_at(self->logs, log_i).ref_i;
	}

	array_count_t j = array_count(self->logs);
	while(j > log_i) {
		--j;
		array_get_at(self->logs, j).ref_i -= ref_i;
	}

	while(j--)
		checkpoint_free(array_get_at(self->logs, j).c);

	array_truncate_first(self->logs, log_i);
	return ref_i;
}
