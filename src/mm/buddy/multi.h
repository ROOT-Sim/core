/**
 * @file mm/buddy/multi.h
 *
 * @brief Handling of multiple buddy systems
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <datatypes/array.h>

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

struct mm_checkpoint {
	uint32_t used_mem;
	unsigned char chkps[];
};

/// Binds a checkpoint together with a reference index
struct mm_log {
	/// The reference index, used to identify this checkpoint
	array_count_t ref_i;
	/// A pointer to the actual checkpoints (contiguous array)
	struct mm_checkpoint *c;
};

/// The checkpointable memory context assigned to a single LP
struct mm_state {
	dyn_array(struct buddy_state *) buddies;
	/// The array of checkpoints
	dyn_array(struct mm_log) logs;
	/// The count of allocated bytes
	uint_fast32_t used_mem;
};

