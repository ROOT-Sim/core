/**
 * @file lp/lp.h
 *
 * @brief LP construction functions
 *
 * LP construction functions
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <gvt/termination.h>
#include <lib/lib.h>
#include <lp/msg.h>
#include <lp/process.h>
#include <mm/model_allocator.h>

/// A complete LP context
struct lp_ctx {
	/// The termination time of this LP, handled by the termination module
	simtime_t t_d;
	/// The additional libraries context of this LP
	struct lib_ctx *lib_ctx_p;
	/// The message processing context of this LP
	struct process_data p;
	
#ifdef RETRACTABILITY
	/// Pointer to this LP's retractable message
	struct _lp_msg* r_msg;
#endif

	/// The memory allocator state of this LP
	struct mm_state mm_state;
};

#define lid_to_nid(lp_id) ((nid_t)((lp_id) * n_nodes / n_lps))
#define lid_to_rid(lp_id) ((rid_t)(((lp_id) - lid_node_first) * n_threads / n_lps_node))

extern uint64_t lid_node_first;
extern __thread uint64_t lid_thread_first;
extern __thread uint64_t lid_thread_end;

extern uint64_t n_lps_node;
extern __thread struct lp_ctx *current_lp;
extern struct lp_ctx *lps;

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);

extern void lp_cleanup(void);

__attribute__ ((pure)) extern lp_id_t lp_id_get_mt(void);
__attribute__ ((pure)) extern struct lib_ctx *lib_ctx_get_mt(void);
