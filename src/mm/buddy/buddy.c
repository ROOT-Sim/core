/**
 * @file mm/buddy/buddy.c
 *
 * @brief A Buddy System implementation
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/buddy.h>

#include <core/core.h>

#define is_power_of_2(i) (!((i) & ((i)-1)))

void buddy_init(struct buddy_state *self)
{
	uint_fast8_t node_size = B_TOTAL_EXP;
	for(uint_fast32_t i = 0; i < sizeof(self->longest) / sizeof(*self->longest); ++i) {
		self->longest[i] = node_size;
		node_size -= is_power_of_2(i + 2);
	}
	self->chunk = distributed_mem_chunk_alloc(self);
}

void buddy_fini(struct buddy_state *self)
{
	distributed_mem_chunk_free(self->chunk);
}

void buddy_moved(struct buddy_state *self)
{
	distributed_mem_chunk_update(self->chunk, self);
}

void *buddy_malloc(struct buddy_state *self, uint_fast8_t req_blks_exp)
{
	if(unlikely(self->longest[0] < req_blks_exp))
		return NULL;

	/* search recursively for the child */
	uint_fast8_t node_size = B_TOTAL_EXP;
	uint_fast32_t i = 0;
	while(node_size > req_blks_exp) {
		/* choose the child with smaller longest value which
		 * is still large at least *size* */
		i = buddy_left_child(i);
		i += self->longest[i] < req_blks_exp;
		--node_size;
	}

	/* update the *longest* value back */
	self->longest[i] = 0;
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

	return self->chunk->mem + offset;
}

uint_fast32_t buddy_free(void *ptr)
{
	struct distr_mem_chunk *chk =
	    (struct distr_mem_chunk *)(((uintptr_t)ptr) & ~(uintptr_t)(sizeof(struct distr_mem_chunk) - 1));
	struct buddy_state *self = distributed_mem_chunk_ref(chk);

	uint_fast8_t node_size = B_BLOCK_EXP;
	uint_fast32_t o = ((uintptr_t)ptr - (uintptr_t)chk->mem) >> B_BLOCK_EXP;
	uint_fast32_t i = o + (1 << (B_TOTAL_EXP - B_BLOCK_EXP)) - 1;

	for(; self->longest[i]; i = buddy_parent(i))
		++node_size;

	self->longest[i] = node_size;
	uint_fast32_t ret = (uint_fast32_t)1U << node_size;
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
	return ret;
}

struct buddy_realloc_res buddy_best_effort_realloc(void *ptr, size_t req_size)
{
	struct distr_mem_chunk *chk =
	    (struct distr_mem_chunk *)(((uintptr_t)ptr) & ~(uintptr_t)(sizeof(struct distr_mem_chunk) - 1));
	struct buddy_state *self = distributed_mem_chunk_ref(chk);

	uint_fast8_t node_size = B_BLOCK_EXP;
	uint_fast32_t o = ((uintptr_t)ptr - (uintptr_t)chk->mem) >> B_BLOCK_EXP;
	uint_fast32_t i = o + (1 << (B_TOTAL_EXP - B_BLOCK_EXP)) - 1;

	for(; self->longest[i]; i = buddy_parent(i))
		++node_size;

	uint_fast8_t req_blks_exp = buddy_allocation_block_compute(req_size);

	struct buddy_realloc_res ret;

	if(node_size == req_blks_exp) {
		// todo: we can do much better than this
		//       for example we can always shrink memory allocations cheaply
		//       we can also enlarge memory allocations without copying if our sibling branches are free

		ret.handled = true;
		ret.variation = 0;
	} else {
		ret.handled = false;
		ret.original = (uint_fast32_t)1U << node_size;
	}

	return ret;
}

void buddy_dirty_mark(const void *ptr, size_t s)
{
	struct distr_mem_chunk *chk =
	    (struct distr_mem_chunk *)(((uintptr_t)ptr) & ~(uintptr_t)(sizeof(struct distr_mem_chunk) - 1));
	struct buddy_state *self = distributed_mem_chunk_ref(chk);
        // TODO: consider using ptrdiff_t here
        uintptr_t diff = (uintptr_t)ptr - (uintptr_t)chk->mem;
	uint_fast32_t i = (diff >> B_BLOCK_EXP) + (1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));

	s += diff & ((1 << B_BLOCK_EXP) - 1);
	s = (s - 1) >> B_BLOCK_EXP;

	do {
		bitmap_set(self->dirty, i + s);
	} while(s--);
}
