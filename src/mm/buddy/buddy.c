/**
 * @file mm/buddy/buddy.c
 *
 * @brief A Buddy System implementation
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/buddy/buddy.h>

#include <core/core.h>

#define is_power_of_2(index) (!((index) & ((index)-1)))

void buddy_init(struct buddy_state *self)
{
	uint_fast8_t node_size = B_TOTAL_EXP;
	for(uint_fast32_t index = 0; index < sizeof(self->longest) / sizeof(*self->longest); ++index) {
		self->longest[index] = node_size;
		node_size -= is_power_of_2(index + 2);
	}
}

void *buddy_malloc(struct buddy_state *self, const uint_fast8_t req_blks_exp)
{
	if(unlikely(self->longest[0] < req_blks_exp))
		return NULL;

	/* search recursively for the child */
	uint_fast8_t node_size = B_TOTAL_EXP;
	uint_fast32_t index = 0;
	while(node_size > req_blks_exp) {
		/* choose the child with smaller longest value which
		 * is still large at least *size* */
		index = buddy_left_child(index);
		index += self->longest[index] < req_blks_exp;
		--node_size;
	}

	/* update the *longest* value back */
	self->longest[index] = 0;
#ifdef ROOTSIM_INCREMENTAL
	bitmap_set(self->dirty, index >> B_BLOCK_EXP);
#endif

	const uint_fast32_t offset = ((index + 1) << node_size) - (1 << B_TOTAL_EXP);

	while(index) {
		index = buddy_parent(index);
		self->longest[index] = max(self->longest[buddy_left_child(index)], self->longest[buddy_right_child(index)]);
#ifdef ROOTSIM_INCREMENTAL
		bitmap_set(self->dirty, index >> B_BLOCK_EXP);
#endif
	}

	return ((char *)self->base_mem) + offset;
}

uint_fast32_t buddy_free(struct buddy_state *self, void *ptr)
{
	uint_fast8_t node_size = B_BLOCK_EXP;
	uint_fast32_t offset = ((uintptr_t)ptr - (uintptr_t)self->base_mem) >> B_BLOCK_EXP;
	uint_fast32_t index = offset + (1 << (B_TOTAL_EXP - B_BLOCK_EXP)) - 1;

	for(; self->longest[index]; index = buddy_parent(index))
		++node_size;

	self->longest[index] = node_size;
	const uint_fast32_t ret = (uint_fast32_t)1U << node_size;
#ifdef ROOTSIM_INCREMENTAL
	bitmap_set(self->dirty, index >> B_BLOCK_EXP);

	uint_fast32_t bitmap_idx = (1 << (node_size - B_BLOCK_EXP)) - 1;
	offset += (1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));
	// need to track freed blocks content because full checkpoints don't
	do {
		bitmap_set(self->dirty, offset + bitmap_idx);
	} while(bitmap_idx--);
#endif

	while(index) {
		index = buddy_parent(index);

		uint_fast8_t left_long = self->longest[buddy_left_child(index)];
		uint_fast8_t right_long = self->longest[buddy_right_child(index)];

		if(left_long == node_size && right_long == node_size) {
			self->longest[index] = node_size + 1;
		} else {
			self->longest[index] = max(left_long, right_long);
		}
#ifdef ROOTSIM_INCREMENTAL
		bitmap_set(self->dirty, i >> B_BLOCK_EXP);
#endif
		++node_size;
	}
	return ret;
}

struct buddy_realloc_res buddy_best_effort_realloc(const struct buddy_state *self, void *ptr, size_t req_size)
{
	uint_fast8_t node_size = B_BLOCK_EXP;
	const uint_fast32_t offset = ((uintptr_t)ptr - (uintptr_t)self->base_mem) >> B_BLOCK_EXP;
	uint_fast32_t index = offset + (1 << (B_TOTAL_EXP - B_BLOCK_EXP)) - 1;

	for(; self->longest[index]; index = buddy_parent(index))
		++node_size;

	const uint_fast8_t req_blks_exp = buddy_allocation_block_compute(req_size);

	struct buddy_realloc_res ret = {0};

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

void buddy_dirty_mark(const struct buddy_state *self, const void *ptr, size_t size)
{
        const uintptr_t diff = ptr - (void *)self->base_mem;
	const uint_fast32_t index = (diff >> B_BLOCK_EXP) + (1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));

	size += diff & ((1 << B_BLOCK_EXP) - 1);
	--size;
	size >>= B_BLOCK_EXP;

	do {
		bitmap_set(self->dirty, index + size);
	} while(size--);
}
