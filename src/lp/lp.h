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

/// A complete LP context
struct lp_ctx {
	struct {
		/// Set to true if the LP is currently locally hosted
		bool local : 1;
		/// The id of the bound thread if @a local is set, the id of the hosting node otherwise
		unsigned id : 31;
	};
	/// The housekeeping epoch number
	unsigned fossil_epoch;
	/// The termination time of this LP, handled by the termination module
	simtime_t termination_t;
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

#ifndef NDEBUG
extern bool lp_initialized;
#define lp_initialized_set() (lp_initialized = true)
#else
#define lp_initialized_set()
#endif

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);
