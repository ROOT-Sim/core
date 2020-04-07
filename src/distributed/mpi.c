#include <distributed/mpi.h>

#include <core/core.h>
#include <core/sync.h>
#include <lp/msg.h>
#include <mm/msg_allocator.h>

#include <mpi.h>

#define MPI_SIM_MSG_TAG 123

#ifndef HAVE_MPI_SERIALIZABLE

#define mpi_lock() 	if (mpi_serialize) spin_lock(&mpi_spinlock)
#define mpi_unlock() 	if (mpi_serialize) spin_unlock(&mpi_spinlock)
#define mpi_trylock() 	(!mpi_serialize || spin_trylock(&mpi_spinlock))

static bool mpi_serialize;
static spinlock_t mpi_spinlock;

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
			rootsim_error(true, "The MPI implementation does not support threads [current thread level support: %d]\n", thread_lvl);
		} else {
#ifndef HAVE_MPI_SERIALIZABLE
			mpi_serialize = true;
			spin_init(&mpi_spinlock);
#else
			rootsim_error(true, "The MPI implementation does not support threads [current thread level support: %d]\n", mpi_thread_lvl_provided);
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
	//todo initial error handling
	;
}

void mpi_global_fini(void)
{
	MPI_Errhandler err_handler;
	MPI_Comm_get_errhandler(MPI_COMM_WORLD, &err_handler);
	MPI_Errhandler_free(&err_handler);

	MPI_Finalize();
}

void mpi_remote_msg_send(const lp_msg *msg)
{
	mpi_lock();
	MPI_Request req;
	MPI_Isend(
		msg,
		msg_bare_size(msg),
		MPI_BYTE,
		lp_id_to_nid(msg->dest),
		MPI_SIM_MSG_TAG,
		MPI_COMM_WORLD,
		&req
	);
	MPI_Request_free(&req);
	mpi_unlock();
}

lp_msg* mpi_remote_msg_rcv(void)
{
	int pending;
	MPI_Message mpi_msg;
	MPI_Status status;

	if(!mpi_trylock())
		return NULL;

	MPI_Improbe(
		MPI_ANY_SOURCE,
		MPI_SIM_MSG_TAG,
		MPI_COMM_WORLD,
		&pending,
		&mpi_msg,
		&status
	);

	if (!pending){
		mpi_unlock();
		return NULL;
	}

	int size;
	MPI_Get_count(&status, MPI_BYTE, &size);
	mpi_unlock();

	lp_msg *ret = msg_allocator_alloc(size);

	mpi_lock();
	MPI_Mrecv(ret, size, MPI_BYTE, &mpi_msg, MPI_STATUS_IGNORE);
	mpi_unlock();

	return ret;
}
