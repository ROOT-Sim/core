/**
* @file mm/buddy/buddy.h
*
* @brief A Buddy System implementation
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include <datatypes/array.h>
#include <datatypes/bitmap.h>

#include <stddef.h>
#include <stdint.h>

#define B_TOTAL_EXP 17U
#define B_BLOCK_EXP 6U
#define B_LOG_INCREMENTAL_THRESHOLD 0.5
#define B_LOG_FREQUENCY 50

struct mm_checkpoint { // todo only log longest[] if changed, or incrementally
#ifdef ROOTSIM_INCREMENTAL
	bool is_incremental;
	block_bitmap dirty [
		bitmap_required_size(
		// this tracks writes to the allocation tree
			(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1)) +
		// while this tracks writes to the actual memory buffer
			(1 << (B_TOTAL_EXP - B_BLOCK_EXP))
		)
	]; /// the dirty bitmap
#endif
	uint_fast32_t used_mem;
	uint_least8_t longest[(1 << (B_TOTAL_EXP - B_BLOCK_EXP + 1))];
	unsigned char base_mem[];
};

struct mm_state { // todo incremental checkpoints
	dyn_array(
		struct mm_log {
			array_count_t ref_i;
			struct mm_checkpoint *c;
		}
	) logs;
	uint_fast32_t used_mem;		/// the count of allocated bytes
	uint_least8_t longest[(1 << (B_TOTAL_EXP - B_BLOCK_EXP + 1))]; // last char is actually unused
	unsigned char base_mem[1 << B_TOTAL_EXP];
#ifdef ROOTSIM_INCREMENTAL
	uint_fast32_t dirty_mem; 	/// the count of dirty bytes
	block_bitmap dirty[
		bitmap_required_size(
		// this tracks writes to the allocation tree
			(1 << (B_TOTAL_EXP - 2 * B_BLOCK_EXP + 1)) +
		// while this tracks writes to the actual memory buffer
			(1 << (B_TOTAL_EXP - B_BLOCK_EXP))

		)
	]; /// the dirty bitmap
#endif
};

_Static_assert(
	offsetof(struct mm_state, longest) ==
	offsetof(struct mm_state, base_mem) -
	sizeof(((struct mm_state *)0)->longest),
	"longest and base_mem are not contiguous, this will break incremental checkpointing");


