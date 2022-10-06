/**
 * @file mm/buddy/multi.h
 *
 * @brief Handling of multiple buddy systems
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

/// The checkpoint for the multiple buddy system allocator
struct mm_checkpoint {
	/// The total count of allocated bytes at the moment of the checkpoint
	uint32_t used_mem;
	/// The sequence of checkpoints of the allocated buddy systems (see @a buddy_checkpoint)
	unsigned char chkps[];
};

/// Binds a checkpoint together with a reference index
struct mm_log {
	/// The reference index, used to identify this checkpoint
	array_count_t ref_i;
	/// A pointer to the actual checkpoint
	struct mm_checkpoint *c;
};

/// The checkpointable memory context assigned to a single LP
struct multi_buddy_state {
	/// The total count of allocated bytes
	uint_fast32_t used_mem;
	/// The array of pointers to the allocated buddy systems for the LP
	dyn_array(struct buddy_state *) buddies;
	/// The array of checkpoints
	dyn_array(struct mm_log) logs;
};

extern void multi_buddy_lp_init(struct multi_buddy_state *m);
extern void multi_buddy_lp_fini(struct multi_buddy_state *m);
extern void *multi_buddy_alloc(struct multi_buddy_state *m, size_t s);
extern void multi_buddy_free(struct multi_buddy_state *m, void *ptr);
extern void *multi_buddy_realloc(struct multi_buddy_state *m, void *ptr, size_t s);
extern void multi_buddy_checkpoint_take(struct multi_buddy_state *m, array_count_t ref_i);
extern void multi_buddy_checkpoint_next_force_full(struct multi_buddy_state *m);
extern array_count_t multi_buddy_checkpoint_restore(struct multi_buddy_state *m, array_count_t ref_i);
extern array_count_t multi_buddy_fossil_lp_collect(struct multi_buddy_state *m, array_count_t tgt_ref_i);
