#include <rebind/migration.h>

#include <core/sync.h>
#include <lp/lp.h>

// TODO: this code will probably be MPI heavy, maybe it's worth to keep this module depending directly on the MPI header
#include <mpi.h>

struct migration {
	uint64_t lp_id;
	int new_nid;
};

static array_declare(struct migration) migrations, migrations_all;
static int recv_counts[MAX_NODES], displs[MAX_NODES];

static void migration_decide(void)
{
	// TODO: compute cross rank migrations and push them to migrations

	// We assume in the rest of the code that migration requests aren't no-ops
	// (i.e.: the current nid of a LP must be different from the newly requested one)
	// and for each LP there's at most a single migration request being enqueued
}

/// Aggregate all the migrations from the MPI ranks so that every rank has a consistent view of the migrations to do
static void migration_disseminate(void)
{
	int sendcount = array_count(migrations) * 2;
	MPI_Allgather(&sendcount, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

	int total = 0;
	for(int i = 0; i < n_nodes; ++i) {
		displs[i] = total;
		total += recv_counts[i];
	}

	array_reserve(migrations_all, total);
	MPI_Allgatherv(array_items(migrations), sendcount, MPI_UINT64_T, array_items(migrations_all), recv_counts,
	    displs, MPI_UINT64_T, MPI_COMM_WORLD);
}

static void migration_lp_send(const struct lp_ctx *lp)
{
	// TODO: implement LP sending logic
}

/// Execute a migration, expected to be called on all ranks for all migrations
static void migration_execute(const struct migration *migration)
{
	struct lp_ctx *lp = &lps[migration->lp_id];

	int new_nid = migration->new_nid;

	if(!LP_RID_IS_NID(lp->rid)) {
		assert(new_nid != nid);
		// TODO migrating a local LP outside

	} else if(new_nid == nid) {
		// TODO expecting a migration from outside
	} else { // not directly involved in this migration
		lp->rid = LP_RID_FROM_NID(new_nid);
	}
}

void migration_on_gvt(void)
{
	// TODO: check if migrations are needed

	if(sync_thread_barrier())
		migration_disseminate();
	sync_thread_barrier();

	// TODO: apply migrations
}
