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
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <distributed/control_msg.h>
#include <lp/msg.h>

extern void mpi_global_init(int *argc_p, char ***argv_p);
extern void mpi_global_fini(void);

extern void mpi_remote_msg_send(struct lp_msg *msg, nid_t dest_nid);
extern void mpi_remote_anti_msg_send(struct lp_msg *msg, nid_t dest_nid);

extern void mpi_control_msg_broadcast(enum msg_ctrl_code ctrl);
extern void mpi_control_msg_send_to(enum msg_ctrl_code ctrl, nid_t dest);
extern void mpi_remote_msg_handle(void);
extern void mpi_remote_msg_drain(void);

extern void mpi_reduce_sum_scatter(const uint32_t values[n_nodes], uint32_t *result);
extern bool mpi_reduce_sum_scatter_done(void);

extern void mpi_reduce_min(double *node_min_p);
extern bool mpi_reduce_min_done(void);

extern void mpi_node_barrier(void);
extern void mpi_blocking_data_send(const void *data, int data_size, nid_t dest);
extern void *mpi_blocking_data_rcv(int *data_size_p, nid_t src);
