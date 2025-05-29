/**
 * @file distributed/mpi.c
 *
 * @brief MPI Support Module
 *
 * This module implements all basic MPI facilities to let the distributed execution of a simulation model take place
 * consistently.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <distributed/mpi.h>
#include <mm/mm.h>

#include <datatypes/msg_queue.h>
#include <mm/msg_allocator.h>

#include <mpi.h>

enum {
	RS_MSG_TAG = 0,
	RS_DATA_TAG
};

/// Array of control codes values to be able to get their address for MPI_Send()
static const enum msg_ctrl_code ctrl_msgs[] = {
	[MSG_CTRL_GVT_START] = MSG_CTRL_GVT_START,
	[MSG_CTRL_GVT_DONE] = MSG_CTRL_GVT_DONE,
	[MSG_CTRL_TERMINATION] = MSG_CTRL_TERMINATION
};

/// The MPI request associated with the non blocking scatter gather collective
static MPI_Request reduce_sum_scatter_req = MPI_REQUEST_NULL;
/// The MPI request associated with the non blocking all reduce collective
static MPI_Request reduce_min_req = MPI_REQUEST_NULL;

/**
 * @brief Handles a MPI error
 * @param comm the MPI communicator involved in the error
 * @param err_code_p a pointer to the error code
 * @param ... an implementation specific list of additional arguments in which
 *            we are not interested
 *
 * This is registered in mpi_global_init() to print meaningful MPI errors
 */
static void comm_error_handler(MPI_Comm *comm, int *err_code_p, ...)
{
	(void)comm;
	logger(LOG_FATAL, "MPI error with code %d!", *err_code_p);

	int err_len;
	char err_str[MPI_MAX_ERROR_STRING];
	MPI_Error_string(*err_code_p, err_str, &err_len);
	logger(LOG_FATAL, "MPI error msg is %s ", err_str);

	exit(-1);
}

/**
 * @brief Initializes the MPI environment
 * @param argc_p a pointer to the OS supplied argc
 * @param argv_p a pointer to the OS supplied argv
 */
void mpi_global_init(int *argc_p, char ***argv_p)
{
	int thread_lvl = MPI_THREAD_SINGLE;
	MPI_Init_thread(argc_p, argv_p, MPI_THREAD_MULTIPLE, &thread_lvl);

	if(thread_lvl < MPI_THREAD_MULTIPLE) {
		logger(LOG_FATAL, "This MPI implementation does not support threaded access");
		abort();
	}

	MPI_Errhandler err_handler;
	if(MPI_Comm_create_errhandler(comm_error_handler, &err_handler)) {
		logger(LOG_FATAL, "Unable to create MPI error handler");
		abort();
	}

	MPI_Comm_set_errhandler(MPI_COMM_WORLD, err_handler);

	int helper;
	MPI_Comm_rank(MPI_COMM_WORLD, &helper);
	nid = helper;
	MPI_Comm_size(MPI_COMM_WORLD, &helper);
	n_nodes = helper;
}

/**
 * @brief Finalizes the MPI environment
 */
void mpi_global_fini(void)
{
	MPI_Errhandler err_handler;
	MPI_Comm_get_errhandler(MPI_COMM_WORLD, &err_handler);
	MPI_Errhandler_free(&err_handler);

	MPI_Finalize();
}

/**
 * @brief Sends a model message to a LP residing on another node
 * @param msg the message to send
 * @param dest_nid the id of the node where the targeted LP resides
 *
 * This function also calls the relevant handlers in order to keep, for example, the non blocking gvt algorithm running.
 * Note that when this function returns, the message may have not been actually sent. We don't need to actively check
 * for sending completion: the platform, during the fossil collection, leverages the gvt to make sure the message has
 * been indeed sent and processed before freeing it.
 */
void mpi_remote_msg_send(struct lp_msg *msg, const nid_t dest_nid)
{
	gvt_remote_msg_send(msg, dest_nid);

	MPI_Request req;
	MPI_Isend(msg_remote_data(msg), msg_remote_size(msg), MPI_BYTE, dest_nid, RS_MSG_TAG, MPI_COMM_WORLD, &req);
	MPI_Request_free(&req);
}

/**
 * @brief Sends a model anti-message to a LP residing on another node
 * @param msg the message to rollback
 * @param dest_nid the id of the node where the targeted LP resides
 *
 * This function also calls the relevant handlers in order to keep, for example, the non blocking gvt algorithm running.
 * Note that when this function returns, the anti-message may have not been sent yet. We don't need to actively check
 * for sending completion: the platform, during the fossil collection, leverages the gvt to make sure the message has
 * been indeed sent and processed before freeing it.
 */
void mpi_remote_anti_msg_send(struct lp_msg *msg, const nid_t dest_nid)
{
	gvt_remote_anti_msg_send(msg, dest_nid);

	MPI_Request req;
	MPI_Isend(msg_remote_data(msg), msg_remote_anti_size(), MPI_BYTE, dest_nid, RS_MSG_TAG, MPI_COMM_WORLD, &req);
	MPI_Request_free(&req);
}

/**
 * @brief Sends a platform control message to all the nodes, including self
 * @param ctrl the control message to send
 */
void mpi_control_msg_broadcast(const enum msg_ctrl_code ctrl)
{
	nid_t i = n_nodes;
	while(i--) {
		mpi_control_msg_send_to(ctrl, i);
	}
}

/**
 * @brief Sends a platform control message to a specific nodes
 * @param ctrl the control message to send
 * @param dest the id of the destination node
 */
void mpi_control_msg_send_to(const enum msg_ctrl_code ctrl, const nid_t dest)
{
	MPI_Request req;
	MPI_Isend(&ctrl_msgs[ctrl], sizeof(*ctrl_msgs), MPI_BYTE, dest, RS_MSG_TAG, MPI_COMM_WORLD, &req);
	MPI_Request_free(&req);
}

/**
 * @brief Empties the queue of incoming MPI messages, doing the right thing for
 *        each one of them.
 *
 * This routine checks, using the MPI probing mechanism, for new remote messages and it handles them accordingly.
 * Control messages are handled by the respective platform handler. Simulation messages are unpacked and put in the
 * queue. Anti-messages are matched and accordingly processed by the message map.
 */
void mpi_remote_msg_handle(void)
{
	while(1) {
		int pending;
		MPI_Message mpi_msg;
		MPI_Status status;

		MPI_Improbe(MPI_ANY_SOURCE, RS_MSG_TAG, MPI_COMM_WORLD, &pending, &mpi_msg, &status);

		if(!pending)
			return;

		int size;
		MPI_Get_count(&status, MPI_BYTE, &size);
		struct lp_msg *msg;
		if(unlikely(size <= (int)msg_remote_anti_size())) {
			if(unlikely(size == sizeof(enum msg_ctrl_code))) {
				enum msg_ctrl_code c;
				MPI_Mrecv(&c, sizeof(c), MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);
				control_msg_process(c);
				continue;
			}
			msg = msg_allocator_alloc(0);
			MPI_Mrecv(msg_remote_data(msg), size, MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);

			gvt_remote_anti_msg_receive(msg);
		} else {
			msg = msg_allocator_alloc(size - offsetof(struct lp_msg, pl) + msg_preamble_size());
			MPI_Mrecv(msg_remote_data(msg), size, MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);

			gvt_remote_msg_receive(msg);
		}
		msg_queue_insert(msg);
	}
}

/**
 * @brief Empties the queue of incoming MPI messages, ignoring them
 *
 * This routine checks, using the MPI probing mechanism, for new remote messages and it discards them. It is used at
 * simulation completion to clear MPI state.
 */
void mpi_remote_msg_drain(void)
{
	struct lp_msg *msg = NULL;
	int msg_size = 0;

	while(1) {
		int pending;
		MPI_Message mpi_msg;
		MPI_Status status;

		MPI_Improbe(MPI_ANY_SOURCE, RS_MSG_TAG, MPI_COMM_WORLD, &pending, &mpi_msg, &status);

		if(!pending)
			break;

		int size;
		MPI_Get_count(&status, MPI_BYTE, &size);

		if(unlikely(size == sizeof(enum msg_ctrl_code))) {
			enum msg_ctrl_code c;
			MPI_Mrecv(&c, sizeof(c), MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);
			control_msg_process(c);
			continue;
		}

		if(size > msg_size) {
			msg = mm_realloc(msg, size + msg_preamble_size());
			msg_size = size;
		}
		MPI_Mrecv(msg_remote_data(msg), size, MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);

		if(size == msg_remote_anti_size())
			gvt_remote_anti_msg_receive(msg);
		else
			gvt_remote_msg_receive(msg);
	}

	mm_free(msg);
}

/**
 * @brief Computes the sum-reduction-scatter operation across all nodes.
 * @param values a flexible array implementing the addendum vector from the calling node.
 * @param result a pointer where the nid-th component of the sum will be stored.
 *
 * Each node supplies a n_nodes components vector. The sum of all these vector is computed and the nid-th component of
 * this vector is stored in @a result. It is expected that only a single thread calls this function at a time. Each node
 * has to call this function else the result can't be computed. It is possible to have a single mpi_reduce_sum_scatter()
 * operation pending at a time. Both arguments must point to valid memory regions until mpi_reduce_sum_scatter_done()
 * returns true.
 */
void mpi_reduce_sum_scatter(const uint32_t values[n_nodes], uint32_t *result)
{
	MPI_Ireduce_scatter_block(values, result, 1, MPI_UINT32_T, MPI_SUM, MPI_COMM_WORLD, &reduce_sum_scatter_req);
}

/**
 * @brief Checks if a previous mpi_reduce_sum_scatter() operation has completed.
 * @return true if the previous operation has been completed, false otherwise.
 */
bool mpi_reduce_sum_scatter_done(void)
{
	int flag = 0;
	MPI_Test(&reduce_sum_scatter_req, &flag, MPI_STATUS_IGNORE);
	return flag;
}

/**
 * @brief Computes the min-reduction operation across all nodes.
 * @param node_min_p a pointer to the value from the calling node which will
 *                   also be used to store the computed minimum.
 *
 * Each node supplies a single simtime_t value. The minimum of all these values is computed and stored in @a node_min_p
 * itself. It is expected that only a single thread calls this function at a time. Each node has to call this function
 * else the result can't be computed. It is possible to have a single mpi_reduce_min() operation pending at a time.
 * Both arguments must point to valid memory regions until mpi_reduce_min_done() returns true.
 */
void mpi_reduce_min(double *node_min_p)
{
	MPI_Iallreduce(MPI_IN_PLACE, node_min_p, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD, &reduce_min_req);
}

/**
 * @brief Checks if a previous mpi_reduce_min() operation has completed.
 * @return true if the previous operation has been completed, false otherwise.
 */
bool mpi_reduce_min_done(void)
{
	int flag = 0;
	MPI_Test(&reduce_min_req, &flag, MPI_STATUS_IGNORE);
	return flag;
}

/**
 * @brief A node barrier
 */
void mpi_node_barrier(void)
{
	MPI_Barrier(MPI_COMM_WORLD);
}

/**
 * @brief Sends a byte buffer to another node
 * @param data a pointer to the buffer to send
 * @param data_size the buffer size
 * @param dest the id of the destination node
 *
 * This operation blocks the execution flow until the destination node receives
 * the data with mpi_raw_data_blocking_rcv().
 */
void mpi_blocking_data_send(const void *data, const int data_size, const nid_t dest)
{
	MPI_Send(data, data_size, MPI_BYTE, dest, RS_DATA_TAG, MPI_COMM_WORLD);
}

/**
 * @brief Receives a byte buffer from another node
 * @param data_size_p where to write the size of the received data
 * @param src the id of the sender node
 * @return the buffer allocated with mm_alloc() containing the received data
 *
 * This operation blocks the execution until the sender node actually sends the data with mpi_raw_data_blocking_send().
 */
void *mpi_blocking_data_rcv(int *data_size_p, const nid_t src)
{
	MPI_Status status;
	MPI_Message mpi_msg;
	MPI_Mprobe(src, RS_DATA_TAG, MPI_COMM_WORLD, &mpi_msg, &status);
	int data_size;
	MPI_Get_count(&status, MPI_BYTE, &data_size);
	char *ret = mm_alloc(data_size);
	MPI_Mrecv(ret, data_size, MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);
	if(data_size_p != NULL)
		*data_size_p = data_size;
	return ret;
}
