/**
 * @file mm/buddy/buddy.h
 *
 * @brief A Buddy System implementation
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/bitmap.h>

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define B_TOTAL_EXP 16U
#define B_BLOCK_EXP 6U

#define next_exp_of_2(i) (sizeof(i) * CHAR_BIT - intrinsics_clz(i))
#define buddy_allocation_block_compute(req_size) next_exp_of_2(max(req_size, 1U << B_BLOCK_EXP) - 1);

#define buddy_left_child(i) (((i) << 1U) + 1U)
#define buddy_right_child(i) (((i) << 1U) + 2U)
#define buddy_parent(i) ((((i) + 1) >> 1U) - 1U)

/// The checkpointable memory context of a single buddy system
struct buddy_state {
	/// The checkpointed binary tree representing the buddy system
	/** the last char is actually unused */
	alignas(16) uint8_t longest[(1U << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	/// The memory buffer served to the model
	alignas(16) unsigned char base_mem[1U << B_TOTAL_EXP];
	/// Keeps track of memory blocks which have been dirtied by a write
	block_bitmap dirty[
		bitmap_required_size(
		// this tracks writes to the allocation tree
			(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1)) +
		// while this tracks writes to the actual memory buffer
			(1 << (B_TOTAL_EXP - B_BLOCK_EXP))
		)
	];
};

static_assert(
	offsetof(struct buddy_state, longest) ==
	offsetof(struct buddy_state, base_mem) -
	sizeof(((struct buddy_state *)0)->longest),
	"longest and base_mem are not contiguous, this will break incremental checkpointing");

extern void buddy_init(struct buddy_state *self);
extern void *buddy_malloc(struct buddy_state *self, uint_fast8_t req_blks_exp);
extern uint_fast32_t buddy_free(struct buddy_state *self, void *ptr);

struct buddy_realloc_res {
	bool handled;
	union {
		int_fast32_t variation;
		uint_fast32_t original;
	};
};
extern struct buddy_realloc_res buddy_best_effort_realloc(const struct buddy_state *self, void *ptr, size_t req_size);
extern void buddy_dirty_mark(const struct buddy_state *self, const void *ptr, size_t size);
