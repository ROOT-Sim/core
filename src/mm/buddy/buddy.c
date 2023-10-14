/**
 * @file mm/buddy/buddy.c
 *
 * @brief A Buddy System implementation
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/buddy.h>

#include <core/core.h>

#define is_power_of_2(i) (!((i) & ((i) - 1U)))

void buddy_init(struct buddy_state *self)
{
	uint_fast8_t node_size = B_TOTAL_EXP - B_BLOCK_EXP + 1;
	self->longest[0] = node_size;
	for(uint_fast32_t i = 1; i < sizeof(self->longest) / sizeof(*self->longest); ++i) {
		node_size -= is_power_of_2(i);
		self->longest[i] = node_size | node_size << 4;
	}
	self->chunk = distributed_mem_chunk_alloc(self);
}

void buddy_fini(struct buddy_state *self)
{
	distributed_mem_chunk_free(self->chunk);
}

void *buddy_malloc(struct buddy_state *self, uint_fast8_t req_blks_exp)
{
	if(unlikely(self->longest[0] + B_BLOCK_EXP - 1 < req_blks_exp))
		return NULL;

	/* search recursively for the child */
	uint_fast8_t node_size = B_TOTAL_EXP; // i = z - 1  z = i + 1
	uint_fast32_t i = 1;
	while(node_size > req_blks_exp) {
		/* choose the child with smaller longest value which
		 * is still large at least *size* */
		i = (i << 1U) + ((self->longest[i] >> 4U) + B_BLOCK_EXP - 1 < req_blks_exp);
		--node_size;
	}
	/* update the *longest* value back */
	self->longest[i >> 1U] &= i & 1U ? 0xf0 : 0x0f;
#ifdef ROOTSIM_INCREMENTAL
	bitmap_set(self->dirty, i >> B_BLOCK_EXP);
#endif

	unsigned char * ret = self->chunk->mem + (i << node_size) - (1 << B_TOTAL_EXP);

	for(i >>= 1; i; i >>= 1) {
		uint_fast8_t c = max(self->longest[i] & 0x0f, self->longest[i] >> 4U);
		uint_fast8_t d = self->longest[i >> 1U];
		self->longest[i >> 1U] = i & 1U ? (d & 0xf0) | c : (d & 0x0f) | (c << 4U);
#ifdef ROOTSIM_INCREMENTAL
		bitmap_set(self->dirty, i >> B_BLOCK_EXP);
#endif
	}

	return ret;
}

uint_fast32_t buddy_free(void *ptr)
{
	struct distr_mem_chunk *chk =
	    (struct distr_mem_chunk *)(((uintptr_t)ptr) & ~(uintptr_t)(sizeof(struct distr_mem_chunk) - 1));
	struct buddy_state *self = distributed_mem_chunk_ref(chk);

	uint_fast8_t node_size = 1;
	uint_fast32_t o = ((uintptr_t)ptr - (uintptr_t)chk->mem) >> B_BLOCK_EXP;
	uint_fast32_t i = o + (1 << (B_TOTAL_EXP - B_BLOCK_EXP));

	for(; self->longest[i >> 1U] & (i & 1U ? 0x0f : 0xf0); i >>= 1U)
		++node_size;

	self->longest[i >> 1U] |= i & 1U ? node_size : node_size << 4U;
	uint_fast32_t ret = (uint_fast32_t)1U << (node_size + B_BLOCK_EXP - 1);
#ifdef ROOTSIM_INCREMENTAL
	bitmap_set(self->dirty, i >> B_BLOCK_EXP);

	uint_fast32_t b = (1 << (node_size - B_BLOCK_EXP)) - 1;
	o += (1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));
	// need to track freed blocks content because full checkpoints don't
	do {
		bitmap_set(self->dirty, o + b);
	} while(b--);
#endif
	for(i >>= 1; i; i >>= 1) {
		uint_fast8_t left_long = self->longest[i] & 0x0f;
		uint_fast8_t right_long = self->longest[i] >> 4;

		uint_fast8_t c;
		if(left_long == node_size && right_long == node_size)
			c = node_size + 1;
		else
			c = max(left_long, right_long);

		uint_fast8_t d = self->longest[i >> 1U];
		self->longest[i >> 1U] = i & 1U ? (d & 0xf0) | c : (d & 0x0f) | (c << 4U);
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
	uint_fast32_t i = o + (1U << (B_TOTAL_EXP - B_BLOCK_EXP));

	for(; self->longest[i >> 1U] & (i & 1U ? 0x0f : 0xf0); i >>= 1U)
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
		//bitmap_set(self->dirty, i + s);
	} while(s--);
}
