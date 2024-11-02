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

#define LP_RID_IS_NID(resource_id) ((resource_id) >> (sizeof(unsigned) * CHAR_BIT - 1))
#define LP_RID_TO_NID(resource_id) (nid_t)((resource_id) & ~(1U << (sizeof(unsigned) * CHAR_BIT - 1)))
#define LP_RID_FROM_NID(node_id) (((unsigned)(node_id)) | (1U << (sizeof(unsigned) * CHAR_BIT - 1)))

/// A complete LP context
struct lp_ctx {
	/// The resource identifier on which this LP is bound TODO: make rid its own struct for safety
	unsigned rid;
	/// The housekeeping epoch number
	unsigned fossil_epoch;
	/// The estimated 'cost' of this LP; the amount of time spent in successful forward execution each GVT phase
	timer_uint cost;
	/// The pointer set by the model with the SetState() API call
	void *state_pointer;
	/// The automatic checkpointing interval selection data
	struct auto_ckpt auto_ckpt;
	/// The message processing context of this LP
	struct process_ctx p;
	/// The memory allocator state of this LP
	struct mm_ctx mm;
	/// A pointer to another LP bound to the same thread, or NULL if this is the last in the chain
	struct lp_ctx *next;
};

extern __thread struct lp_ctx *thread_first_lp;
extern __thread struct lp_ctx *current_lp;
extern struct lp_ctx *lps;

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);
