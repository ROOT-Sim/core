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
#include <lp/lp.h>

/**
 * @brief Compute termination operations after the receipt of a termination control message
  FIXME: buggy if some nodes/threads apply the termination time update in different parts of their GVT commit phase
 */
void termination_on_ctrl_msg(void)
{
	global_config.termination_time = -1.0;
}

/**
 * @brief Update the termination module state after a GVT computation
 *
 * Here we check if the simulation, from the point of view of the current thread, can be terminated. If also all the
 * other processing threads on the node are willing to end the simulation, a termination control message is broadcast to
 * the other nodes.
 */
bool termination_on_gvt(simtime_t current_gvt)
{
	return current_gvt >= global_config.termination_time;
}

/**
 * @brief Force termination of the simulation
 */
void RootsimStop(void)
{
	if(global_config.serial) {
		global_config.termination_time = -1.0;
		return;
	}

	nid_t i = n_nodes + 1;
	while(i--)
		mpi_control_msg_broadcast(MSG_CTRL_TERMINATION);
}
