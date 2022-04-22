/**
 * @file gvt/termination.c
 *
 * @brief Termination detection module
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <gvt/termination.h>

#include <distributed/mpi.h>
#include <lp/lp.h>

_Atomic nid_t nodes_to_end;
static _Atomic rid_t thr_to_end;
static __thread uint64_t lps_to_end;
static __thread simtime_t max_t;

/**
 * @brief Initialize the termination detection module node-wide
 */
void termination_global_init(void)
{
	atomic_store_explicit(&thr_to_end, global_config.n_threads, memory_order_relaxed);
	atomic_store_explicit(&nodes_to_end, n_nodes, memory_order_relaxed);
}

/**
 * @brief Initialize the termination detection module LP-wide
 */
void termination_lp_init(void)
{
	struct lp_ctx *this_lp = current_lp;
	bool term = global_config.committed(this_lp - lps, current_lp->lib_ctx->state_s);
	lps_to_end += !term;
	this_lp->t_d = term * SIMTIME_MAX;
}

/**
 * @brief Compute termination operations after a new message has been processed
 * @param msg_time the timestamp of the freshly processed message
 */
void termination_on_msg_process(simtime_t msg_time)
{
	struct lp_ctx *this_lp = current_lp;
	if(this_lp->t_d)
		return;

	bool term = global_config.committed(this_lp - lps, this_lp->lib_ctx->state_s);
	max_t = term ? max(msg_time, max_t) : max_t;
	this_lp->t_d = term * msg_time;
	lps_to_end -= term;
}

/**
 * @brief Compute termination operations after the receipt of a termination control message
 */
void termination_on_ctrl_msg(void)
{
	atomic_fetch_sub_explicit(&nodes_to_end, 1U, memory_order_relaxed);
}

/**
 * @brief Compute termination operations after the receipt of a termination control message
 */
void termination_on_gvt(simtime_t current_gvt)
{
	if(likely((lps_to_end || max_t >= current_gvt) && current_gvt < global_config.termination_time))
		return;
	max_t = SIMTIME_MAX;
	unsigned t = atomic_fetch_sub_explicit(&thr_to_end, 1U, memory_order_relaxed);
	if(t == 1)
		mpi_control_msg_broadcast(MSG_CTRL_TERMINATION);
}

/**
 * @brief Force termination of the simulation
 *
 * FIXME this doesn't actually work: concurrent termination messages will break this
 */
void termination_force(void)
{
	nid_t i = atomic_load_explicit(&nodes_to_end, memory_order_relaxed);
	while(i--)
		mpi_control_msg_broadcast(MSG_CTRL_TERMINATION);
}

/**
 * @brief Compute termination operations after a LP has been rollbacked
 * @param msg_time the timestamp of the straggler or anti message which caused the rollback
 */
void termination_on_lp_rollback(simtime_t msg_time)
{
	struct lp_ctx *this_lp = current_lp;
	simtime_t old_t = this_lp->t_d;
	bool keep = old_t < msg_time || old_t == SIMTIME_MAX;
	this_lp->t_d = keep * old_t;
	lps_to_end += !keep;
}
