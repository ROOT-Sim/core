/**
 * @file mm/buddy/multi.h
 *
 * @brief Handling of multiple buddy systems
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
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
	uint_fast32_t ckpt_size;
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
struct mm_state {
	/// The array of pointers to the allocated buddy systems for the LP
	dyn_array(struct buddy_state *) buddies;
	/// The array of checkpoints
	dyn_array(struct mm_log) logs;
	/// The total count of allocated bytes
	uint_fast32_t full_ckpt_size;
};
