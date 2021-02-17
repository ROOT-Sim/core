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
	/// The memory allocator state of this LP
	struct mm_state mm_state;
};

#define lps_iter_init(start_i, lps_cnt)					\
{									\
	const unsigned __i = rid;					\
	const unsigned __ucnt = n_threads;				\
	const uint64_t __lpscnt = (nid + 1 == n_nodes) ? 		\
		n_lps - n_lps_node * (n_nodes - 1) : n_lps_node;	\
	const bool __big = __i >= (__ucnt - (__lpscnt % __ucnt));	\
	start_i = n_lps_node * nid;					\
	start_i += __big ?						\
		__lpscnt - ((__lpscnt / __ucnt + 1) * (__ucnt - __i)) : \
		((__lpscnt / __ucnt) * __i);				\
	lps_cnt	= (__lpscnt / __ucnt) + __big;				\
}

extern uint64_t n_lps_node;
extern __thread struct lp_ctx *current_lp;
extern struct lp_ctx *lps;
extern rid_t *lid_to_rid;

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);

extern void lp_cleanup(void);

__attribute__ ((pure)) extern lp_id_t lp_id_get_mt(void);
__attribute__ ((pure)) extern struct lib_ctx *lib_ctx_get_mt(void);
