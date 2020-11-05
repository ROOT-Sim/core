/**
* @file distributed/mpi.c
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
#include <distributed/mpi.h>

#ifdef ROOTSIM_MPI

#include <core/core.h>
#include <core/sync.h>
#include <datatypes/array.h>
#include <datatypes/msg_queue.h>
#include <datatypes/remote_msg_map.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include <mm/msg_allocator.h>

#include <mpi.h>

#ifdef ROOTSIM_MPI_SERIALIZABLE

static bool mpi_serialize;
static spinlock_t mpi_spinlock;

#define mpi_lock() 	if (mpi_serialize) spin_lock(&mpi_spinlock)
#define mpi_unlock() 	if (mpi_serialize) spin_unlock(&mpi_spinlock)
#define mpi_trylock() 	(!mpi_serialize || spin_trylock(&mpi_spinlock))

#else

#define mpi_lock()
#define mpi_unlock()
#define mpi_trylock()	true

#endif

static void comm_error_handler(MPI_Comm *comm, int *err_code_p, ...)
{
	(void) comm;
	log_log(LOG_FATAL, "MPI error with code %d!", *err_code_p);

	int err_len;
	char err_str[MPI_MAX_ERROR_STRING];
	MPI_Error_string(*err_code_p, err_str, &err_len);
	log_log(LOG_FATAL, "MPI error msg is %s ", err_str);

	abort();
}

void mpi_global_init(int *argc_p, char ***argv_p)
{
	int thread_lvl = MPI_THREAD_SINGLE;
	MPI_Init_thread(argc_p, argv_p, MPI_THREAD_MULTIPLE, &thread_lvl);

	if (thread_lvl < MPI_THREAD_MULTIPLE) {
		if (thread_lvl < MPI_THREAD_SERIALIZED) {
			log_log(LOG_FATAL,
				"This MPI implementation does not support threaded access");
			abort();
		} else {
#ifdef ROOTSIM_MPI_SERIALIZABLE
			mpi_serialize = true;
			spin_init(&mpi_spinlock);
#else
			log_log(LOG_FATAL,
				"This MPI implementation only supports serialized calls: you need to build ROOT-Sim with -Dserialized_mpi=true");
			abort();
#endif
		}
	}

	MPI_Errhandler err_handler;
	if (MPI_Comm_create_errhandler(comm_error_handler, &err_handler)){
		log_log(LOG_FATAL, "Unable to create MPI error handler");
		abort();
	}

	MPI_Comm_set_errhandler(MPI_COMM_WORLD, err_handler);

	int helper;
	MPI_Comm_rank(MPI_COMM_WORLD, &helper);
	nid = helper;
	MPI_Comm_size(MPI_COMM_WORLD, &helper);
	n_nodes = helper;
}

void mpi_global_fini(void)
{
	MPI_Errhandler err_handler;
	MPI_Comm_get_errhandler(MPI_COMM_WORLD, &err_handler);
	MPI_Errhandler_free(&err_handler);

	MPI_Finalize();
}

void mpi_remote_msg_send(struct lp_msg *msg, nid_t dest_nid)
{
	bool phase = gvt_phase_get();
	msg->msg_id = msg_id_get(msg, phase);
	msg_id_phase_set(msg->msg_id, phase);
	gvt_on_remote_msg_send(dest_nid);

	mpi_lock();
	MPI_Request req;
	MPI_Isend(msg, msg_bare_size(msg), MPI_BYTE, dest_nid, 0,
		MPI_COMM_WORLD, &req);
	MPI_Request_free(&req);
	mpi_unlock();
}

void mpi_remote_anti_msg_send(struct lp_msg *msg, nid_t dest_nid)
{
	msg_id_phase_set(msg->msg_id, gvt_phase_get());
	gvt_on_remote_msg_send(dest_nid);

	mpi_lock();
	MPI_Request req;
	MPI_Issend(&msg->msg_id, sizeof(msg->msg_id), MPI_BYTE, dest_nid, 0,
		MPI_COMM_WORLD, &req);
	MPI_Request_free(&req);
	mpi_unlock();
}

void mpi_control_msg_broadcast(enum _msg_ctrl ctrl)
{
	MPI_Request req;
	nid_t i = n_nodes;
	mpi_lock();
	while (i--) {
		if(i == nid)
			continue;
		MPI_Isend(NULL, 0, MPI_BYTE, i, ctrl, MPI_COMM_WORLD, &req);
		MPI_Request_free(&req);
	}
	mpi_unlock();
}

void mpi_control_msg_send_to(enum _msg_ctrl ctrl, nid_t dest)
{
	MPI_Request req;
	mpi_lock();
	MPI_Isend(NULL, 0, MPI_BYTE, dest, ctrl, MPI_COMM_WORLD, &req);
	MPI_Request_free(&req);
	mpi_unlock();
}

void mpi_remote_msg_handle(void)
{
	int pending;
	MPI_Message mpi_msg;
	MPI_Status status;

	do {
		if(!mpi_trylock())
			return;

		MPI_Improbe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
			&pending, &mpi_msg, &status);

		if (!pending){
			mpi_unlock();
			return;
		}

		if (unlikely(status.MPI_TAG)) {
			MPI_Mrecv(NULL, 0, MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);
			mpi_unlock();
			switch(status.MPI_TAG){
			case MSG_CTRL_GVT_START:
				gvt_on_start_msg();
				break;
			case MSG_CTRL_GVT_DONE:
				gvt_on_done_msg();
				break;
			case MSG_CTRL_TERMINATION:
				termination_on_ctrl_msg();
				break;
			}
		} else {
			int size;
			MPI_Get_count(&status, MPI_BYTE, &size);

			if (unlikely(size == sizeof((struct lp_msg *)0)->msg_id)) {
				uintptr_t anti_id;
				MPI_Mrecv(&anti_id, size, MPI_BYTE, &mpi_msg,
					MPI_STATUS_IGNORE);
				mpi_unlock();

				remote_msg_map_match(anti_id,
					status.MPI_SOURCE, NULL);
				gvt_on_remote_msg_receive(
					msg_id_phase_get(anti_id));
			} else {
				mpi_unlock();

				struct lp_msg *msg = msg_allocator_alloc(size -
					offsetof(struct lp_msg, pl));

				mpi_lock();
				MPI_Mrecv(msg, size, MPI_BYTE, &mpi_msg,
					MPI_STATUS_IGNORE);
				mpi_unlock();

				uintptr_t msg_id = msg->msg_id;
				atomic_store_explicit(&msg->flags, 0U,
					memory_order_relaxed);
				remote_msg_map_match(msg_id,
					status.MPI_SOURCE, msg);
				msg_queue_insert(msg);
				gvt_on_remote_msg_receive(
					msg_id_phase_get(msg_id));
			}
		}
	} while (1);
}

static MPI_Request reduce_remote_sent_req = MPI_REQUEST_NULL;

bool mpi_reduce_remote_sent_done(void)
{
	int flag = 0;
	mpi_lock();
	MPI_Test(&reduce_remote_sent_req, &flag, MPI_STATUS_IGNORE);
	mpi_unlock();
	return flag;
}

void mpi_reduce_remote_sent(const unsigned *sent_msgs, unsigned *result)
{
	mpi_lock();
	MPI_Ireduce_scatter_block(sent_msgs, result, 1, MPI_UNSIGNED, MPI_SUM,
		MPI_COMM_WORLD, &reduce_remote_sent_req);
	mpi_unlock();
}

static MPI_Request reduce_local_min_req = MPI_REQUEST_NULL;

bool mpi_reduce_local_min_done(void)
{
	int flag = 0;
	mpi_lock();
	MPI_Test(&reduce_local_min_req, &flag, MPI_STATUS_IGNORE);
	mpi_unlock();
	return flag;
}

void mpi_reduce_local_min(simtime_t *local_min_p)
{
	static simtime_t min_buff;
	min_buff = *local_min_p;
	mpi_lock();
	MPI_Iallreduce(&min_buff, local_min_p, 1, MPI_DOUBLE, MPI_MIN,
		MPI_COMM_WORLD, &reduce_local_min_req);
	mpi_unlock();
}

#endif
