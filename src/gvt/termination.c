/**
 * @file gvt/termination.c
 *
 * @brief Termination detection module
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <gvt/termination.h>

#include <distributed/mpi.h>
#include <lp/lp.h>

/// The number of nodes that still need to continue running the simulation
_Atomic nid_t nodes_to_end;
/// The number of local threads that still need to continue running the simulation
static _Atomic rid_t thr_to_end;
/// The number of thread-locally bounded LPs that still need to continue running the simulation
static __thread uint64_t lps_to_end;
/// The maximum speculative time at which a thread-local LP declared its intention to terminate
/** FIXME: a wrong high termination time during a speculative trajectory forces the simulation to uselessly continue */
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
void termination_lp_init(struct lp_ctx *lp)
{
	bool term = global_config.committed(lp - lps, lp->state_pointer);
	lps_to_end += !term;
	lp->termination_t = term * SIMTIME_MAX;
}

/**
 * @brief Compute termination operations after a new message has been processed
 * @param msg_time the timestamp of the freshly processed message
 */
void termination_on_msg_process(struct lp_ctx *lp, simtime_t msg_time)
{
	if(lp->termination_t)
		return;

	bool term = global_config.committed(lp - lps, lp->state_pointer);
	max_t = term ? max(msg_time, max_t) : max_t;
	lp->termination_t = term * msg_time;
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
 * @brief Update the termination module state after a GVT computation
 *
 * Here we check if the simulation, from the point of view of the current thread, can be terminated. If also all the
 * other processing threads on the node are willing to end the simulation, a termination control message is broadcast to
 * the other nodes.
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
 */
void RootsimStop(void)
{
	if(global_config.synchronization == SERIAL) {
		global_config.termination_time = -1.0;
		return;
	}

	nid_t i = n_nodes + 1;
	while(i--)
		mpi_control_msg_broadcast(MSG_CTRL_TERMINATION);
}

/**
 * @brief Compute termination operations after a LP has been rollbacked
 * @param msg_time the timestamp of the straggler or anti message which caused the rollback
 */
void termination_on_lp_rollback(struct lp_ctx *lp, simtime_t msg_time)
{
	simtime_t old_t = lp->termination_t;
	bool keep = old_t < msg_time || old_t == SIMTIME_MAX;
	lp->termination_t = keep * old_t;
	lps_to_end += !keep;
}
