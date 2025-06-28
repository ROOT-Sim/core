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

/// Tells if the given index is a power of 2
#define is_power_of_2(idx) (!((idx) & ((idx) - 1)))

/**
 * @brief Initializes the buddy system allocator.
 *
 * This function sets up the buddy system allocator by initializing the `longest` array
 * in the `buddy_state` structure. Each entry in the array represents the size of the largest
 * free block in the corresponding subtree of the buddy system.
 *
 * @param self Pointer to the `buddy_state` structure to initialize.
 */
void buddy_init(struct buddy_state *self)
{
	uint_fast8_t node_size = B_TOTAL_EXP - B_BLOCK_EXP + 1;
	self->longest[0] = node_size;
	for(uint_fast32_t i = 1; i < sizeof(self->longest) / sizeof(*self->longest); ++i) {
		node_size -= is_power_of_2(i);
		self->longest[i] = node_size | node_size << 4;
	}
}

/**
 * @brief Allocates memory using the buddy system allocator.
 *
 * This function allocates a memory block of the requested size (expressed as a power of 2)
 * from the buddy system. It searches for the smallest suitable block, marks it as used,
 * and updates the internal state of the allocator.
 *
 * @param self Pointer to the `buddy_state` structure representing the buddy system allocator.
 * @param req_blks_exp The size of the requested memory block, expressed as a power of 2.
 * @return A pointer to the allocated memory block, or `NULL` if no suitable block is available.
 */
void *buddy_malloc(struct buddy_state *self, const uint_fast8_t req_blks_exp)
{
	if(unlikely(self->longest[0] + B_BLOCK_EXP - 1 < req_blks_exp))
		return NULL;

	/* search recursively for the child */
	uint_fast8_t node_size = B_TOTAL_EXP; // i = z - 1  z = i + 1
	uint_fast32_t idx = 1;
	while(node_size > req_blks_exp) {
		idx = (idx << 1U) + ((self->longest[idx] >> 4U) + B_BLOCK_EXP - 1 < req_blks_exp);
		/* choose the child with smaller longest value which is still large at least *size* */
		--node_size;
	}
	self->longest[idx >> 1U] &= idx & 1U ? 0xf0 : 0x0f;

	unsigned char * const ret = self->base_mem + (idx << node_size) - (1 << B_TOTAL_EXP);

	for(idx >>= 1; idx; idx >>= 1) {
		uint_fast8_t c = max(self->longest[idx] & 0x0f, self->longest[idx] >> 4U);
		uint_fast8_t d = self->longest[idx >> 1U];
		self->longest[idx >> 1U] = idx & 1U ? (d & 0xf0) | c : (d & 0x0f) | (c << 4U);
	}

	return ret;
}


/**
 * @brief Frees a memory block allocated by the buddy system allocator.
 *
 * This function releases a previously allocated memory block back to the buddy system.
 * It updates the internal state of the allocator to reflect the newly freed block
 * and merges adjacent free blocks if possible.
 *
 * @param self Pointer to the `buddy_state` structure representing the buddy system allocator.
 * @param ptr Pointer to the memory block to be freed.
 * @return The size of the freed memory block in bytes.
 */
uint_fast32_t buddy_free(struct buddy_state *self, void *ptr)
{
	uint_fast8_t node_size = 1;
	const uint_fast32_t offset = ((uintptr_t)ptr - (uintptr_t)self->base_mem) >> B_BLOCK_EXP;
	uint_fast32_t idx = offset + (1 << (B_TOTAL_EXP - B_BLOCK_EXP));

	for(; self->longest[idx >> 1U] & (idx & 1U ? 0x0f : 0xf0); idx >>= 1U)
		++node_size;

	self->longest[idx >> 1U] |= idx & 1U ? node_size : node_size << 4U;
	const uint_fast32_t ret = (uint_fast32_t)1U << (node_size + B_BLOCK_EXP - 1);

	for(idx >>= 1; idx; idx >>= 1) {
		uint_fast8_t left_long = self->longest[idx] & 0x0f;
		uint_fast8_t right_long = self->longest[idx] >> 4;

		uint_fast8_t c;
		if(left_long == node_size && right_long == node_size)
			c = node_size + 1;
		else
			c = max(left_long, right_long);

		uint_fast8_t d = self->longest[idx >> 1U];
		self->longest[idx >> 1U] = idx & 1U ? (d & 0xf0) | c : (d & 0x0f) | (c << 4U);
		++node_size;
	}
	return ret;
}


/**
 * @brief Attempts to reallocate memory with the buddy system allocator.
 *
 * This function tries to resize a memory block allocated by the buddy system
 * to the requested size. If the requested size matches the current size, the
 * operation is handled without any changes. Otherwise, it determines whether
 * the reallocation can be performed and provides information about the
 * original size of the block.
 *
 * @param self Pointer to the `buddy_state` structure representing the buddy system allocator.
 * @param ptr Pointer to the memory block to be reallocated.
 * @param req_size The requested size for the memory block in bytes.
 * @return A `buddy_realloc_res` structure containing the result of the reallocation attempt:
 *         - `handled`: Indicates whether the reallocation was handled.
 *         - `variation`: The size difference if the reallocation was handled.
 *         - `original`: The original size of the memory block if the reallocation was not handled.
 */
struct buddy_realloc_res buddy_best_effort_realloc(const struct buddy_state *self, void *ptr, size_t req_size)
{
	uint_fast8_t node_size = B_BLOCK_EXP;
	const uint_fast32_t offset = ((uintptr_t)ptr - (uintptr_t)self->base_mem) >> B_BLOCK_EXP;
	uint_fast32_t idx = offset + (1U << (B_TOTAL_EXP - B_BLOCK_EXP));

	for(; self->longest[idx >> 1U] & (idx & 1U ? 0x0f : 0xf0); idx >>= 1U)
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

/**
 * @brief Marks a memory region as dirty for incremental checkpointing.
 *
 * This function marks a specified memory region as dirty in the buddy system allocator.
 * The dirty marking is used to track changes to memory blocks for incremental checkpointing.
 * Note: Incremental checkpointing is currently not functioning.
 *
 * @param self Pointer to the `buddy_state` structure representing the buddy system allocator.
 * @param ptr Pointer to the start of the memory region to be marked as dirty.
 * @param size The size of the memory region to be marked as dirty, in bytes.
 */
void buddy_dirty_mark(const struct buddy_state *self, const void *ptr, size_t size)
{
	const uintptr_t diff = (unsigned char *)ptr - (unsigned char *)self->base_mem;
	const uint_fast32_t idx = (diff >> B_BLOCK_EXP) + (1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1));

	size += diff & ((1 << B_BLOCK_EXP) - 1);
	--size;
	size >>= B_BLOCK_EXP;

	do {
		bitmap_set(self->dirty, idx + size);
	} while(size--);
}
