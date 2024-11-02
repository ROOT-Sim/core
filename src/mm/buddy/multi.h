/**
 * @file mm/buddy/multi.h
 *
 * @brief Handling of multiple buddy systems
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>

#include <mm/buddy/buddy.h>

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

/// The checkpoint for the multiple buddy system allocator
struct mm_checkpoint {
	/// The total count of allocated bytes at the moment of the checkpoint
	uint_fast32_t checkpoints_size;
	/// The sequence of checkpoints of the allocated buddy systems (see @a buddy_checkpoint)
	unsigned char checkpoints[];
};

/// Binds a checkpoint together with a reference index
struct mm_log {
	/// The reference index, used to identify this checkpoint
	array_count_t ref_i;
	/// A pointer to the actual checkpoint
	struct mm_checkpoint *c;
};

/// A list of structs @a buddy_state
struct mm_buddy_list {
	/// Pointer to next element in the list
	struct mm_buddy_list *next;
	/// The buddy_state struct in this list node
	struct buddy_state buddy;
};

/// The checkpointable memory context assigned to a single LP
struct mm_ctx {
	/// The array of pointers to the allocated buddy systems for the LP
	struct mm_buddy_list *buddies;
	/// The array of checkpoints
	array_declare(struct mm_log) logs;
	/// The total count of allocated bytes
	uint_fast32_t full_ckpt_size;
};
