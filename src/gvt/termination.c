/**
 * @file gvt/termination.c
 *
 * @brief Termination detection module
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <gvt/termination.h>

#include <distributed/mpi.h>
#include <gvt/gvt.h>
#include <lp/lp.h>

#define TERMINATION_COMMITTED (-1.0)
#define TERMINATION_NOT_READY (-2.0)

/// The number of nodes that still need to continue running the simulation
static _Atomic lp_id_t lps_to_end;
/// The gvt phase
static _Atomic int termination_gvt_phase = -1;

/**
 * @brief Initialize the termination detection module node-wide
 */
void termination_global_init(void)
{
	atomic_store_explicit(&lps_to_end, global_config.lps, memory_order_relaxed);
}

/**
 * @brief Initialize the termination detection module LP-wide
 */
void termination_lp_init(struct lp_ctx *lp)
{
	lp->termination_t = TERMINATION_NOT_READY;
}

/**
 * @brief Compute termination operations after a new message has been processed
 * @param lp the LP that has just processed a message
 * @param msg_time the timestamp of the freshly processed message
 */
void termination_on_msg_process(struct lp_ctx *lp, simtime_t msg_time)
{
	if(unlikely(lp->termination_t >= TERMINATION_COMMITTED))
		return;

	if(unlikely(lp->termination_t == TERMINATION_REQUESTED)) {
		lp->termination_t = msg_time;
		return;
	}

	const bool term = global_config.committed(lp - lps, lp->state_pointer);
	if(term)
		lp->termination_t = msg_time;
}

/**
 * @brief Update the termination module state after a GVT computation
 *
 * Here we check if the simulation, from the point of view of the current thread, can be terminated. If also all the
 * other processing threads on the node are willing to end the simulation, a termination control message is broadcast to
 * the other nodes.
 */
bool termination_on_gvt(const simtime_t current_gvt)
{
	return unlikely(current_gvt >= global_config.termination_time ||
			atomic_load_explicit(&termination_gvt_phase, memory_order_relaxed) == (int)gvt_phase);
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

	for(lp_id_t remaining = atomic_load_explicit(&lps_to_end, memory_order_relaxed); remaining--;)
		mpi_control_msg_send_to(MSG_CTRL_LP_END_TERMINATION, n_nodes - 1);
}

/**
 * @brief Potentially invalidate termination state after a LP rolled back
 * @param lp the LP that has been rolled back
 * @param msg_time the timestamp of the straggler or anti message that caused the rollback
 */
void termination_on_lp_rollback(struct lp_ctx *lp, const simtime_t msg_time)
{
	if(lp->termination_t >= msg_time)
		lp->termination_t = TERMINATION_NOT_READY;
}

/**
 * @brief Compute termination operations after a LP has been rolled back
 * @param lp the LP that has been rolled back
 * @param msg_time the timestamp of the straggler or anti message that caused the rollback
 */
void termination_on_lp_fossil(struct lp_ctx *lp, const simtime_t gvt_time)
{
	if(likely(lp->termination_t < 0.0 || lp->termination_t >= gvt_time))
		return;

	lp->termination_t = TERMINATION_COMMITTED;
	mpi_control_msg_send_to(MSG_CTRL_LP_END_TERMINATION, n_nodes - 1);
}

/**
 * @brief Process a LP termination control message
 */
void termination_on_lp_end_ctrl_msg(void)
{
	lp_id_t left = atomic_fetch_sub_explicit(&lps_to_end, 1U, memory_order_relaxed);
	if(likely(left != 1 || nid != n_nodes - 1))
		return;

	// The simulation can finally end. We use GVT-tracked messages so that we can make sure MPI ranks will be
	// exiting the processing loop at the same (logically committed) time
	for(nid_t i = n_nodes; i;)
		++remote_msg_seq[gvt_phase][--i];

	mpi_control_msg_broadcast(gvt_phase ? MSG_CTRL_PHASE_ORANGE_TERMINATION : MSG_CTRL_PHASE_PURPLE_TERMINATION);
}

/**
 * @brief Handle the termination process triggered by orange phase global termination control message
 */
void termination_on_orange_end_ctrl_msg(void)
{
	++remote_msg_received[1U];
	atomic_store_explicit(&termination_gvt_phase, 0U, memory_order_relaxed);
}

/**
 * @brief Handle the termination process triggered by purple phase global termination control message
 */
void termination_on_purple_end_ctrl_msg(void)
{
	++remote_msg_received[0U];
	atomic_store_explicit(&termination_gvt_phase, 1U, memory_order_relaxed);
}

void rs_termination_request(void)
{
	if(current_lp->termination_t < 0)
		current_lp->termination_t = TERMINATION_REQUESTED;
}
