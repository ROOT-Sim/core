/**
 * @file mm/checkpoint/checkpoint.h
 *
 * @brief Header of the model allocator checkpointing subsystem
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <mm/model_allocator.h>
#include <inttypes.h>

#ifdef ROOTSIM_INCREMENTAL
/// Tells whether a checkpoint is incremental or not.
#define is_log_incremental(l) ((uintptr_t)(l).c & 0x1)
#else
/// Tells whether a checkpoint is incremental or not.
#define is_log_incremental(l) false
#endif


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
	array_count_t ref_idx;
	/// A pointer to the actual checkpoint
	struct mm_checkpoint *ckpt;
};

/// Structure to keep data used for autonomic checkpointing selection
struct auto_ckpt {
	/// The inverse of the rollback probability
	double inv_bad_p;
	/// The count of straggler and anti-messages
	unsigned m_bad;
	/// The count of correctly processed forward messages
	unsigned m_good;
	/// The currently selected checkpointing interval
	unsigned ckpt_interval;
	/// The count of remaining events to process until the next checkpoint
	unsigned ckpt_rem;
};

/**
 * Register a "bad" message, i.e. one message which cause a rollback
 * @param auto_ckpt a pointer to the auto-checkpoint module struct of the current LP
 */
#define auto_ckpt_register_bad(auto_ckpt) ((auto_ckpt)->m_bad++)

/**
 * Register a "good" message, i.e. one message processed in forward execution
 * @param auto_ckpt a pointer to the auto-checkpoint module struct of the current LP
 */
#define auto_ckpt_register_good(auto_ckpt) ((auto_ckpt)->m_good++)

/**
 * Get the currently computed optimal checkpointing interval
 * @param auto_ckpt a pointer to the auto-checkpoint module struct of the current LP
 * @return the optimal checkpointing interval for the current LP
 */
#define auto_ckpt_interval_get(auto_ckpt) ((auto_ckpt)->ckpt_interval)

/**
 * Register a new processed message and check if, for the given LP, a checkpoint is necessary
 * @param auto_ckpt a pointer to the auto-checkpoint module struct of the current LP
 * @return true if the LP needs a checkpoint, false otherwise
 */
#define auto_ckpt_is_needed(auto_ckpt)                                                                                 \
	__extension__({                                                                                                \
		_Bool r = ++(auto_ckpt)->ckpt_rem >= (auto_ckpt)->ckpt_interval;                                       \
		if(r)                                                                                                  \
			(auto_ckpt)->ckpt_rem = 0;                                                                     \
		r;                                                                                                     \
	})

extern void auto_ckpt_init(void);
extern void auto_ckpt_lp_init(struct auto_ckpt *auto_ckpt);
extern void auto_ckpt_on_gvt(void);
extern void auto_ckpt_recompute(struct auto_ckpt *auto_ckpt, uint_fast32_t state_size);
extern void model_allocator_checkpoint_next_force_full(const struct mm_state *self);
extern void model_allocator_checkpoint_take(struct mm_state *self, array_count_t ref_idx);
extern array_count_t model_allocator_checkpoint_restore(struct mm_state *self, array_count_t ref_idx);
