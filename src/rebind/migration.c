#include <rebind/migration.h>

#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <gvt/fossil.h>
#include <gvt/gvt.h>
#include <mm/msg_allocator.h>

#include <assert.h>

typedef array_declare(char) byte_array;

struct migration {
	uint64_t lp_id;
	int new_nid;
};

static array_declare(struct migration) migrations, locally_decided_migrations;

static void migration_decide(void)
{
	// TODO: compute cross rank migrations and push them to locally_decided_migrations

	// We assume in the rest of the code that migration requests aren't no-ops
	// (i.e.: the current nid of a LP must be different from the newly requested one)
	// and for each LP there's at most a single migration request being enqueued
}

/**
 * @brief Aggregate all the migrations from MPI ranks so that every rank has a consistent view of the migrations to do
 */
static void migration_disseminate(void)
{
	static int recv_counts[MAX_NODES], displs[MAX_NODES];

	int sendcount = array_count(locally_decided_migrations) * 2;
	mpi_allgather_int_single(sendcount, recv_counts);

	int total = 0;
	for(nid_t i = 0; i < n_nodes; ++i) {
		displs[i] = total;
		total += recv_counts[i];
	}

	array_reserve(migrations, total);
	mpi_allgatherv_u64(sendcount, (uint64_t *)array_items(locally_decided_migrations), recv_counts,
	    (uint64_t *)array_items(migrations), displs);
}

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

/**
 * @brief Transform a local message into a remote message, if needed
 *
 * During normal execution, messages exchanged in shared memory haven't a global ID since they can be anti-messaged
 * without (see https://doi.org/10.1145/3615979.3656053 ).
 * When migrated, these messages will need a proper ID, to become anti-message-able also across different MPI ranks.
 * This code will attempt to make a message remote if it is not, otherwise it will schedule the message for deferred
 * deletion. Notice that this function may be called two times for a single message (this happens if a local message
 * has been sent from a local LP to another local LP and both of the LP have to migrate). Potentially this double call
 * may happen concurrently in different threads, hence we have to handle this case with atomics.
 */
static void migrating_msg_to_remote_transform(struct lp_msg *msg)
{
	uint64_t flags = atomic_load_explicit(&msg->flags, memory_order_relaxed), new_flags;
	do {
		if(flags >> 2) {
			// the message is remote (or it has been made remote during this migration)
			// we can safely get rid of it in the future, because
			msg_allocator_deferred_free(msg);
			return;
		}

		new_flags = flags | gvt_msg_id_generate();
	} while(atomic_compare_exchange_weak_explicit(&msg->flags, &flags, new_flags, memory_order_relaxed,
	    memory_order_relaxed));
}

static byte_array migration_lp_serialize(struct lp_ctx *lp)
{
	assert(lp->p.early_antis == NULL);

	byte_array data;
	array_init(data);
	array_push_many(data, lp_plain_data(lp), lp_plain_data_size());

	// Serialize messages in the queue
	array_count_t count = 0;
	array_count_t count_loc = array_count(data);
	array_push_many(data, &count, sizeof(count));

	array_count_t iter_state = ARRAY_COUNT_MAX;
	bool to_serialize; // if set to true, the iterator call will remove the message from the queue
	for(struct lp_msg *msg = msg_queue_iter(false, &iter_state); msg;
	    msg = msg_queue_iter(to_serialize, &iter_state)) {
		to_serialize = lp == lps + msg->dest;

		if(!to_serialize)
			continue;

		if(lp->p.early_antis != NULL && process_early_anti_message_check(&lp->p, msg))
			// this is an early anti-message we need to get rid of
			continue;

		uint64_t flags = atomic_load_explicit(&msg->flags, memory_order_relaxed);
		if((flags & MSG_FLAG_ANTI) != 0 && (flags & MSG_FLAG_PROCESSED) == 0) {
			// this is a timely local anti-message; we can safely get completely rid of it
			msg_allocator_free(msg);
			continue;
		}

		migrating_msg_to_remote_transform(msg);
		array_push_many(data, msg_remote_data(msg), msg_remote_size(msg));
		++count;
	}
	memcpy(&array_get_at(data, count_loc), &count, sizeof(count));

	// Serialize messages in the past event set
	array_push_many(data, &array_count(lp->p.pes), sizeof(array_count(lp->p.pes)));

	for(array_count_t i = 0; i < array_count(lp->p.pes); ++i) {
		struct pes_entry e = array_get_at(lp->p.pes, i);

		unsigned char is_received = pes_entry_is_received(e);
		array_push_many(data, &is_received, sizeof(is_received));

		struct lp_msg *msg = pes_entry_msg(e);
		migrating_msg_to_remote_transform(msg);
		array_push_many(data, msg_remote_data(msg), msg_remote_size(msg));
	}

	array_count_t mm_dump_size = model_allocator_serialize_size(&lp->mm);
	array_reserve(data, mm_dump_size);

	void *end_address = &array_get_at(data, array_count(data));
	model_allocator_serialize_dump(&lp->mm, end_address);

	return data;
}

static void migration_lp_receive(struct lp_ctx *lp, const void *lp_data)
{
	const char *ptr = lp_data;

	// basic LP init
	lp->rid = tid;
	array_init(lp->p.pes);
	lp->p.early_antis = NULL;
	lp->p.bound = -1.0;

	memcpy(lp_plain_data(lp), ptr, lp_plain_data_size());
	ptr += lp_plain_data_size();

	array_count_t idx;
	memcpy(&idx, ptr, sizeof(idx));
	ptr += sizeof(idx);

	while(idx--) {
		array_count_t pl_size;
		memcpy(&pl_size, ptr + offsetof(struct lp_msg, pl_size) - msg_preamble_size(), sizeof(pl_size));
		struct lp_msg *msg = msg_allocator_alloc(pl_size);

		array_count_t msg_size = msg_remote_size(msg);
		memcpy(msg_remote_data(msg), ptr, msg_size);
		ptr += msg_size;

		msg_queue_insert(msg, lp->rid);
	}

	memcpy(&idx, ptr, sizeof(idx));
	ptr += sizeof(idx);

	while(idx--) {
		unsigned char is_received = *ptr++;

		array_count_t pl_size;
		memcpy(&pl_size, ptr + offsetof(struct lp_msg, pl_size) - msg_preamble_size(), sizeof(pl_size));
		struct lp_msg *msg = msg_allocator_alloc(pl_size);

		array_count_t msg_size = msg_remote_size(msg);
		memcpy(msg_remote_data(msg), ptr, msg_size);
		ptr += msg_size;

		struct pes_entry entry = pes_entry_make(msg, is_received ? PES_ENTRY_RECEIVED : PES_ENTRY_SENT_REMOTE);
		array_push(lp->p.pes, entry);
	}
}

static void lps_serialize(void)
{

	for(struct lp_ctx **prev_p = &thread_first_lp, *lp = *prev_p; lp; lp = *prev_p) {
		if(lp->rid == tid)
			continue;

		*prev_p = lp->next;
		lp->next = lp; // Signal to the next phase that this LP has been serialized

		// Serialize the LP to be migrated and release its data structures
		byte_array data = migration_lp_serialize(lp);
		// TODO: LP cleanup, really TODO
		// TODO: remove this hack. Reusing LP fields as temporary storage
		lp->state_pointer = array_items(data);
		lp->cost = array_count(data);
	}
}

static void lps_serialized_send_receive(void)
{
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

}

static void lps_msgs_sent_align(void)
{
	for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next) {
		if(lp->rid != tid)
			continue;

		// Scan sent messages and mark them remote if they have a remote id
		for(array_count_t i = 0; i < array_count(lp->p.pes); ++i) {
			struct pes_entry e = array_get_at(lp->p.pes, i);
			if(!pes_entry_is_sent_local(e))
				continue;

			struct lp_msg *msg = pes_entry_msg(e);
			if(msg->raw_flags >> 2)
				array_get_at(lp->p.pes, i) = pes_entry_make(msg, PES_ENTRY_SENT_REMOTE);
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

	lps_serialize();

	sync_thread_barrier();

	lps_serialized_send_receive();

	sync_thread_barrier();

	lps_msgs_sent_align();
}
