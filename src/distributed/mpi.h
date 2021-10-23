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
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <lp/msg.h>

/// A control message MPI tag value
enum msg_ctrl_tag {
	/// Used by the master to start a new gvt reduction operation
	MSG_CTRL_GVT_START = 1,
	/// Used by slaves to signal their completion of the gvt protocol
	MSG_CTRL_GVT_DONE,
	/// Used in broadcast to signal that local LPs can terminate
	MSG_CTRL_TERMINATION,
#ifdef PUBSUB
    MSG_PUBSUB
#endif
};

extern void mpi_global_init(int *argc_p, char ***argv_p);
extern void mpi_global_fini(void);

extern void mpi_remote_msg_send(struct lp_msg *msg, nid_t dest_nid);
extern void mpi_remote_anti_msg_send(struct lp_msg *msg, nid_t dest_nid);

extern void mpi_control_msg_broadcast(enum msg_ctrl_tag ctrl);
extern void mpi_control_msg_send_to(enum msg_ctrl_tag ctrl, nid_t dest);
extern void mpi_remote_msg_handle(void);

extern void mpi_reduce_sum_scatter(const unsigned values[n_nodes],
		unsigned *result);
extern bool mpi_reduce_sum_scatter_done(void);

extern void mpi_reduce_min(double *node_min_p);
extern bool mpi_reduce_min_done(void);

extern void mpi_node_barrier(void);
extern void mpi_blocking_data_send(const void *data, int data_size, nid_t dest);
extern void *mpi_blocking_data_rcv(int *data_size_p, nid_t src);
