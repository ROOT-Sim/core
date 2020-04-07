#include <core/sync.h>

#include <core/core.h>

static atomic_uint c_1;		///< First synchronization counter
static atomic_uint c_2;		///< Second synchronization counter
static atomic_uint c_leader;	/**< "Barrier in a barrier": this is used to
 	 	 	 	 * wait for the leader to correctly reset
 	 	 	 	 * the barrier before re-entering */

void sync_global_init(void)
{
	unsigned t_cnt = n_threads;
	atomic_store_explicit(&c_2, t_cnt, memory_order_release);
	atomic_store_explicit(&c_1, t_cnt, memory_order_release);
	atomic_store_explicit(&c_leader, 0U, memory_order_release);
}

bool sync_thread_barrier(void)
{
	// Wait for the leader to finish resetting the barrier
	while (atomic_load_explicit(&c_leader, memory_order_seq_cst)) {}

	// Wait for all threads to synchronize
	atomic_fetch_sub_explicit(&c_1, 1U, memory_order_seq_cst);

	while (atomic_load_explicit(&c_1, memory_order_seq_cst)) {}

	// Leader election
	if (!atomic_fetch_add_explicit(&c_leader, 1U, memory_order_seq_cst)) {

		atomic_fetch_sub_explicit(&c_2, 1U, memory_order_seq_cst);

		// Wait all the other threads to leave the barrier
		while (atomic_load_explicit(&c_2, memory_order_seq_cst)) {}

		// Reset the barrier to its initial values
		unsigned t_cnt = n_threads;
		atomic_store_explicit(&c_2, t_cnt, memory_order_seq_cst);
		atomic_store_explicit(&c_1, t_cnt, memory_order_seq_cst);
		atomic_store_explicit(&c_leader, 0U, memory_order_seq_cst);

		return true;
	}
	// I'm sync'ed!
	atomic_fetch_sub_explicit(&c_2, 1U, memory_order_seq_cst);

	return false;
}
