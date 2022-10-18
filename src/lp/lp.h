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
#include <lib/lib.h>
#include <lp/msg.h>
#include <lp/process.h>
#include <mm/auto_ckpt.h>
#include <mm/model_allocator.h>

/// A complete LP context
struct lp_ctx {
	/// The termination time of this LP, handled by the termination module
	simtime_t termination_t;
	/// The additional libraries context of this LP
	struct lib_ctx *lib_ctx;
	/// The automatic checkpointing interval selection data
	struct auto_ckpt auto_ckpt;
	/// The message processing context of this LP
	struct process_data p;
	/// The memory allocator state of this LP
	struct mm_state mm_state;
};

/**
 * @brief Compute the id of the node which hosts a given LP
 * @param lp_id the id of the LP
 * @return the id of the node which hosts the LP identified by @p lp_id
 */
#define lid_to_nid(lp_id) ((nid_t)((lp_id) * n_nodes / global_config.lps))

/**
 * @brief Compute the id of the thread which hosts a given LP
 * @param lp_id the id of the LP
 * @return the id of the thread which hosts the LP identified by @p lp_id
 *
 * Horrible things may happen if @p lp_id is not locally hosted (use #lid_to_nid() to make sure of that!)
 */
#define lid_to_rid(lp_id) ((rid_t)(((lp_id) - lid_node_first) * global_config.n_threads / n_lps_node))

extern uint64_t lid_node_first;
extern __thread uint64_t lid_thread_first;
extern __thread uint64_t lid_thread_end;

extern __thread struct lp_ctx *current_lp;
extern struct lp_ctx *lps;

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);

extern void lp_on_gvt(simtime_t gvt);

_pure extern lp_id_t lp_id_get(void);
_pure extern struct lib_ctx *lib_ctx_get(void);
