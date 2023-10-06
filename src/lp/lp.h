/**
 * @file lp/lp.h
 *
 * @brief LP construction functions
 *
 * LP construction functions
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <arch/platform.h>
#include <core/core.h>
#include <lp/msg.h>
#include <lp/process.h>
#include <mm/auto_ckpt.h>
#include <mm/model_allocator.h>

#define LP_RID_IS_NID(processor_id) ((processor_id) >> (sizeof(unsigned) * CHAR_BIT - 1))
#define LP_RID_TO_NID(processor_id) (nid_t)((processor_id) & ~(1U << (sizeof(unsigned) * CHAR_BIT - 1)))
#define LP_RID_FROM_NID(this_nid) (((unsigned)(this_nid)) | (1U << (sizeof(unsigned) * CHAR_BIT - 1)))

/// A complete LP context
struct lp_ctx {
	unsigned rid;
	/// The housekeeping epoch number
	unsigned fossil_epoch;
	/// The pointer set by the model with the SetState() API call
	void *state_pointer;
	/// The automatic checkpointing interval selection data
	struct auto_ckpt auto_ckpt;
	/// The message processing context of this LP
	struct process_ctx p;
	/// The memory allocator state of this LP
	struct mm_state mm_state;
};

extern __thread struct lp_ctx *current_lp;
extern struct lp_ctx *lps;

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);
