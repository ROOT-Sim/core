/**
 * @file lp/lp.h
 *
 * @brief LP construction functions
 *
 * LP construction functions
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <arch/platform.h>
#include <core/core.h>
#include <lib/random/random.h>
#include <lp/msg.h>
#include <lp/process.h>
#include <mm/auto_ckpt.h>
#include <mm/model_allocator.h>

/// A complete LP context
struct lp_ctx {
	/// The termination time of this LP, handled by the termination module
	simtime_t termination_t;
	/// The additional libraries context of this LP
	struct rng_ctx *rng_ctx;
	/// The pointer set by the model with the SetState() API call
	void *state_pointer;
	/// The housekeeping epoch number
	unsigned fossil_epoch;
	/// The automatic checkpointing interval selection data
	struct auto_ckpt auto_ckpt;
	/// The message processing context of this LP
	struct process_ctx p;
	/// The memory allocator state of this LP
	struct mm_state mm_state;
};

/**
 * @brief Compute the id of the node which hosts a given LP
 * @param lp_id the id of the LP
 * @return the id of the node which hosts the LP identified by @p lp_id
 */
#define lid_to_nid(lp_id) 0

/**
 * @brief Compute the id of the thread which hosts a given LP
 * @param lp_id the id of the LP
 * @return the id of the thread which hosts the LP identified by @p lp_id
 *
 * Horrible things may happen if @p lp_id is not locally hosted (use #lid_to_nid() to make sure of that!)
 */
#define lid_to_rid_old(lp_id) ((rid_t)(((lp_id)-lid_node_first) * global_config.n_threads / n_lps_node))

extern uint64_t lid_node_first;
extern __thread uint64_t lid_thread_first;
extern __thread uint64_t lid_thread_end;

extern __thread struct lp_ctx *current_lp;
extern struct lp_ctx *lps;

static inline rid_t lid_to_rid(lp_id_t lp_id)
{
	if(lp_id < global_config.lps_racer) {
		return lp_id * global_config.n_threads_racer / global_config.lps_racer;
	} else {
		return global_config.n_threads_racer +
		       (lp_id - global_config.lps_racer) * global_config.n_threads_warp / global_config.lps_warp;
	}
}

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
