#include <rebind/migration.h>

#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/fossil.h>
#include <gvt/gvt.h>
#include <mm/msg_allocator.h>

#include <assert.h>
// TODO: this code will probably be MPI heavy, maybe it's worth to keep this module depending directly on the MPI header
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

/**
 * @brief Aggregate all the migrations from MPI ranks so that every rank has a consistent view of the migrations to do
 */
static void migration_disseminate(void)
{
	int sendcount = array_count(migrations) * 2;
	mpi_allgather_int_single(sendcount, recv_counts);

	int total = 0;
	for(nid_t i = 0; i < n_nodes; ++i) {
		displs[i] = total;
		total += recv_counts[i];
	}

	array_reserve(migrations_all, total);
	mpi_allgatherv_u64(sendcount, (uint64_t *)array_items(migrations), recv_counts,
	    (uint64_t *)array_items(migrations_all), displs);}

/**
 * @brief Update the rid of LPs that are migrating
 *
 * Here the rid of LPs involved in migrations is updated with the new one. This includes LPs that are migrating into the
 * current node; near the end of migration operations processing threads will get ownership of LPs that migrated in by
 * updating once again their rid.
 */
static void lps_rid_update(void)
{
	for(array_count_t i = tid; i < array_count(migrations); i += global_config.n_threads) {
		struct migration m = array_get_at(migrations, i);
		struct lp_ctx *lp = &lps[m.lp_id];
		// assert there aren't external no-op migrations
		assert(lp->rid != LP_RID_FROM_NID(m.new_nid));
		// assert there aren't incoming no-op migrations
		assert((m.new_nid != nid) || LP_RID_IS_NID(lp->rid));
		lp->rid = LP_RID_FROM_NID(m.new_nid);
	}
}

static void queue_msgs_detach(void)
{
	array_count_t i = ARRAY_COUNT_MAX;
	bool removed; // if set to true, the iterator call will get rid of the message
	for(struct lp_msg *msg = msg_queue_iter(false, &i); msg; msg = msg_queue_iter(removed, &i)) {
		removed = false;

		struct lp_ctx *lp = &lps[msg->dest];
		if(lp->rid == tid)
			// this LP is not leaving the current thread (and the current node)
			continue;

		if(lp->p.early_antis != NULL && process_early_anti_message_check(&lp->p, msg)) {
			// this is an early anti-message we need to get rid of
			removed = true;
			continue;
		}

		uint64_t flags = msg->raw_flags;
		if(flags >> 2) {
			// this message is coming from another node and will be serialized and migrated,
			// therefore we will get rid of the buffer on this node
			msg_allocator_free_at_gvt(msg);
		} else if((flags & MSG_FLAG_ANTI) != 0 && (flags & MSG_FLAG_PROCESSED) == 0) {
			// this is a timely local anti-message; we can safely get completely rid of it
			msg_allocator_free(msg);
			removed = true;
		} else {
			// this is a locally sent message: we need to give it an ID; we don't free the msg because it is
			// bound to a local LP that will need it to, possibly, anti-message the event after migrations
			gvt_msg_id_assign(msg);
		}
	}
}

/**
 * @brief Assign a remote id to locally received messages
 * @param lp a pointer to the LP whose locally received messages need to be detached
 *
 * During normal execution, messages exchanged in shared memory haven't a global ID since they can be anti-messaged
 * without (see https://doi.org/10.1145/3615979.3656053 ).
 * When migrated, these messages will need a proper ID, to become anti-message-able also in distributed execution.
 * Here we take care of local messages that have been either _sent_ or _received_ by LPs that are migrating away; other
 * parts of code will deal with the other tricky edge cases
 */
static inline void lp_msgs_detach(bool received)
{
	for(array_count_t j = tid; j < array_count(migrations); j += global_config.n_threads) {
		struct migration m = array_get_at(migrations, j);
		struct lp_ctx *lp = &lps[m.lp_id];
		for(array_count_t i = 0; i < array_count(lp->p.pes); ++i) {
			struct pes_entry e = array_get_at(lp->p.pes, i);
			if(pes_entry_is_received(e) != received)
				continue;

			struct lp_msg *msg = pes_entry_msg(e);
			if(msg->raw_flags >> 2U)
				msg_allocator_free_at_gvt(msg);
			else
				gvt_msg_id_assign(msg);
		}
	}
}


static size_t migration_lp_size(const struct lp_ctx *lp)
{
	assert(lp->p.early_antis == NULL);

	size_t ret = lp_plain_data_size(lp);
	return ret;
}

static void migration_lp_serialize(const struct lp_ctx *lp, void *lp_data)
{
	assert(lp->p.early_antis == NULL);

	char *ptr = lp_data;
	size_t plain_data_size = lp_plain_data_size(lp);
	memcpy(ptr, lp_plain_data(lp), plain_data_size);
	ptr += plain_data_size;

	memcpy(ptr, &array_count(lp->p.pes), sizeof(array_count(lp->p.pes)));
	ptr += sizeof(array_count(lp->p.pes));

	for(array_count_t i = 0; i < array_count(lp->p.pes); ++i) {
		struct pes_entry e = array_get_at(lp->p.pes, i);

		unsigned char r = pes_entry_is_received(e);
		memcpy(ptr, &r, sizeof(r));
		ptr += sizeof(r);

		struct lp_msg *msg = pes_entry_msg(e);
		size_t msg_size = msg_remote_size(msg);
		memcpy(ptr, msg, msg_size);
		ptr += msg_size;
	}

	array_count_t count = 0, iter_state = ARRAY_COUNT_MAX;
	char *count_loc = ptr;
	ptr += sizeof(count);
	struct lp_msg *msg;
	while((msg = msg_queue_iter(true, &iter_state)))
		++count;

	memcpy(count_loc, &count, sizeof(count));

	model_allocator_serialize_dump(&lp->mm, ptr);

	// send basic lp data

	// send past event set
	// - past event set will contain local messages that may need to be roll-backed
	//   local threads need to be involved to generate remote ids for those messages
	//   (actually, maybe the thread executing the migration can do that)
	//   anti-messages are a PITA since they would require to be handled according to the NB state machine
	// - remote past events that will become local do not need special handling

	// send mm subsystem
	// -
}

static void migration_lp_receive(struct lp_ctx *lp, const void *lp_data)
{
	// TODO: implement LP receive logic
}

static void lp_sent_check_or_serialize(void)
{
	for(struct lp_ctx **prev_p = &thread_first_lp, *lp = *prev_p; lp; lp = *prev_p) {
		if(lp->rid == tid) {
			prev_p = &lp->next;
			// scan sent messages and mark them remote if they have a remote id
			for(array_count_t i = 0; i < array_count(lp->p.pes); ++i) {
				struct pes_entry e = array_get_at(lp->p.pes, i);
				if(!pes_entry_is_sent_local(e))
					continue;

				struct lp_msg *msg = pes_entry_msg(e);
				if(msg->raw_flags >> 2)
					array_get_at(lp->p.pes, i) = pes_entry_make(msg, PES_ENTRY_SENT_REMOTE);
			}
		} else {
			*prev_p = lp->next;
			lp->next = lp; // Signal to the next phase that this LP has been serialized

			// Serialize the LP to be migrated and release its data structures
			size_t lp_data_size = migration_lp_size(lp);
			void *lp_data = mm_alloc(lp_data_size);
			migration_lp_serialize(lp, lp_data);
			// TODO: LP cleanup, really TODO
			// TODO: remove this hack. Reusing LP fields as temporary storage
			lp->state_pointer = lp_data;
			lp->cost = lp_data_size;
		}
	}
}

void migration_on_gvt(void)
{
	// TODO: check if migrations are needed

	// fossil collection of all the LPs
	for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next)
		fossil_lp_collect(lp);

	// gather migrations from all the nodes, only a single thread will do that
	static atomic_flag disseminated = ATOMIC_FLAG_INIT;
	if(!atomic_flag_test_and_set_explicit(&disseminated, memory_order_relaxed))
		migration_disseminate();

	if(sync_thread_barrier())
		atomic_flag_clear_explicit(&disseminated, memory_order_relaxed);

	lps_rid_update();

	gvt_msg_barrier();

	queue_msgs_detach();

	sync_thread_barrier();

	lp_msgs_detach(true);

	sync_thread_barrier();

	lp_msgs_detach(false);

	sync_thread_barrier();

	lp_sent_check_or_serialize();

	sync_thread_barrier();

	for(array_count_t i = tid; i < array_count(migrations); i += global_config.n_threads) {
		struct migration m = array_get_at(migrations, i);
		struct lp_ctx *lp = &lps[m.lp_id];

		if(m.new_nid == nid) {
			void *lp_data = mpi_blocking_data_rcv(NULL, -1);
			migration_lp_receive(lp, lp_data);
			mm_free(lp_data);

			lp->next = thread_first_lp;
			thread_first_lp = lp;

		} else if(lp->next == lp) {
			mpi_blocking_data_send(lp->state_pointer, lp->cost, m.new_nid);
			mm_free(lp->state_pointer);
			lp->rid = LP_RID_FROM_NID(m.new_nid);
			lp->next = NULL;
		}
	}

	sync_thread_barrier();
	
}
