#include <distributed/mpi.h>

#include <core/core.h>
#include <core/sync.h>
#include <datatypes/array.h>
#include <datatypes/msg_queue.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include <mm/msg_allocator.h>

#ifdef HAVE_MPI

#include <mpi.h>

#ifndef HAVE_MPI_SERIALIZABLE

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
	//todo proper MPI error handling
}

void mpi_global_init(int *argc_p, char ***argv_p)
{
	int thread_lvl = MPI_THREAD_SINGLE;
	MPI_Init_thread(argc_p, argv_p, MPI_THREAD_MULTIPLE, &thread_lvl);

	if (thread_lvl < MPI_THREAD_MULTIPLE) {
		if (thread_lvl < MPI_THREAD_SERIALIZED) {
			log_log(LOG_FATAL, "This MPI implementation does not support threads\n");
			goto error;
		} else {
#ifndef HAVE_MPI_SERIALIZABLE
			mpi_serialize = true;
			spin_init(&mpi_spinlock);
#else
			log_log(LOG_FATAL, "This MPI implementation only supports serialized calls: you need to compile NeuRome with --enable-serialized-mpi\n");
			goto error;
#endif
		}
	}

	MPI_Errhandler err_handler;
	if (MPI_Comm_create_errhandler(comm_error_handler, &err_handler))
		goto error;

	MPI_Comm_set_errhandler(MPI_COMM_WORLD, err_handler);

	int this_nid;
	MPI_Comm_rank(MPI_COMM_WORLD, &this_nid);
	nid = this_nid;

	return;
	error:
	abort();
}

void mpi_global_fini(void)
{
	MPI_Errhandler err_handler;
	MPI_Comm_get_errhandler(MPI_COMM_WORLD, &err_handler);
	MPI_Errhandler_free(&err_handler);

	MPI_Finalize();
}

void mpi_remote_msg_send(lp_msg *msg)
{
	mpi_lock();
	MPI_Request req;
	MPI_Issend(msg, msg_bare_size(msg), MPI_BYTE, lp_id_to_nid(msg->dest),
		0, MPI_COMM_WORLD, &req);
	MPI_Request_free(&req);
	mpi_unlock();
	msg_allocator_free_at_gvt(msg);
}

void mpi_control_msg_broadcast(enum _msg_ctrl ctrl)
{
	mpi_lock();
	MPI_Request req;
	unsigned i = n_nodes;
	while(i--){
		if(i == nid)
			continue;
		MPI_Issend(NULL, 0, MPI_BYTE, i, 0, MPI_COMM_WORLD, &req);
		MPI_Request_free(&req);
	}
	mpi_unlock();
}

void mpi_remote_msg_handle(void)
{
	int pending;
	MPI_Message mpi_msg;
	MPI_Status status;

	do{
		if(!mpi_trylock())
			return;

		MPI_Improbe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
			&pending, &mpi_msg, &status);

		if (!pending){
			mpi_unlock();
			return;
		}

		if(unlikely(status.MPI_TAG)){
			MPI_Mrecv(NULL, 0, MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);
			mpi_unlock();
			if(status.MPI_TAG){
			case MSG_CTRL_GVT:
				gvt_on_ctrl_msg();
				break;
			case MSG_CTRL_TERMINATION:
				termination_on_ctrl_msg();
				break;
			}
		} else {
			int size;
			MPI_Get_count(&status, MPI_BYTE, &size);
			mpi_unlock();

			lp_msg *msg = msg_allocator_alloc(size);

			mpi_lock();
			MPI_Mrecv(msg, size, MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);
			mpi_unlock();

			msg_queue_insert(msg);
		}
	} while(1);
}

#endif
