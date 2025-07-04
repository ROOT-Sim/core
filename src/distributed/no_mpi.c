/**
 * @file distributed/no_mpi.c
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

#include <distributed/mpi.h>

#include <assert.h>

void mpi_global_init(int *argc_p, char ***argv_p)
{
	(void)argc_p;
	(void)argv_p;
}

void mpi_global_fini(void) {}

void mpi_remote_msg_send(struct lp_msg *msg, nid_t dest_nid)
{
	(void)msg;
	(void)dest_nid;
	assert(0);
	__builtin_unreachable();
}

void mpi_remote_anti_msg_send(struct lp_msg *msg, nid_t dest_nid)
{
	(void)msg;
	(void)dest_nid;
	assert(0);
	__builtin_unreachable();
}

void mpi_control_msg_broadcast(enum msg_ctrl_code ctrl)
{
	control_msg_process(ctrl);
}

void mpi_control_msg_send_to(enum msg_ctrl_code ctrl, nid_t dest)
{
	assert(dest == 0);
	if(dest)
		__builtin_unreachable();

	mpi_control_msg_broadcast(ctrl);
}

void mpi_remote_msg_handle(void) {}

void mpi_remote_msg_drain(void) {}

void mpi_reduce_sum_scatter(const uint32_t values[n_nodes], uint32_t *result)
{
	*result = values[0];
}

bool mpi_reduce_sum_scatter_done(void)
{
	return true;
}

void mpi_reduce_min(double *node_min_p)
{
	(void)node_min_p;
}

bool mpi_reduce_min_done(void)
{
	return true;
}

void mpi_node_barrier(void) {}

void mpi_blocking_data_send(const void *data, int data_size, nid_t dest)
{
	(void)data;
	(void)data_size;
	(void)dest;
}

void *mpi_blocking_data_rcv(int *data_size_p, nid_t src)
{
	(void)data_size_p;
	(void)src;
	return NULL;
}
