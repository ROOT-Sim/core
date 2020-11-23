/**
* @file distributed/mpi.h
*
* @brief MPI Support Module
*
* This module implements all basic MPI facilities to let the distributed
* execution of a simulation model take place consistently.
*
* Several facilities are thread-safe, others are not. Check carefully which
* of these can be used by worker threads without coordination when relying
* on this module.
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#ifdef ROOTSIM_MPI

#include <lp/msg.h>

enum _msg_ctrl {
	MSG_CTRL_GVT_START = 1,
	MSG_CTRL_GVT_DONE,
	MSG_CTRL_TERMINATION
};

extern void mpi_global_init(int *argc_p, char ***argv_p);
extern void mpi_global_fini(void);

extern void mpi_remote_msg_send(struct lp_msg *msg, nid_t dest_nid);
extern void mpi_remote_anti_msg_send(struct lp_msg *msg, nid_t dest_nid);

extern void mpi_control_msg_broadcast(enum _msg_ctrl ctrl);
extern void mpi_control_msg_send_to(enum _msg_ctrl ctrl, nid_t dest);
extern void mpi_remote_msg_handle(void);

extern void mpi_reduce_sum_scatter(const unsigned node_vals[n_nodes], unsigned *result);
extern bool mpi_reduce_sum_scatter_done(void);

extern void mpi_reduce_min(simtime_t *node_min_p);
extern bool mpi_reduce_min_done(void);

#endif
