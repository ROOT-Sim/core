/**
 * @file lp/lp.c
 *
 * @brief LP construction functions
 *
 * LP construction functions
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lp/lp.h>

#include <datatypes/msg_queue.h>
#include <core/sync.h>
#include <gvt/fossil.h>
#include <gvt/termination.h>

/// The lowest LP id between the ones hosted on this node
uint64_t lid_node_first;
/// The lowest LP id between the ones hosted on this thread
__thread uint64_t lid_thread_first;
/// One plus the highest LP id between the ones hosted on this thread
__thread uint64_t lid_thread_end;
/// A pointer to the currently processed LP context
__thread struct lp_ctx *current_lp;
/// A pointer to the LP contexts array
/** Valid entries are contained between #lid_node_first and #lid_node_first + #n_lps_node - 1, limits included */
struct lp_ctx *lps;
/// The number of LPs hosted on this node
lp_id_t n_lps_node;

/**
 * @brief Compute the first index of a partition in a linear space of indexes
 * @param part_id the id of the requested partition
 * @param part_cnt the number of requested partitions
 * @param part_fnc the function which computes the partition id from an index
 * @param start_i the first valid index of the space to partition
 * @param tot_i the size of the index space to partition
 *
 * The description is somewhat confusing but this does a simple thing. Each LP has an integer id and we want each
 * node/processing unit to have more or less the same number of LPs assigned to it; this macro computes the first index
 * of the LP to assign to a node/processing unit.
 */
#define partition_start(part_id, part_cnt, part_fnc, start_i, tot_i)                                                   \
	__extension__({                                                                                                \
		lp_id_t _g = (part_id) * (tot_i) / (part_cnt) + (start_i);                                             \
		while(_g > (start_i) && part_fnc(_g) >= (part_id))                                                     \
			--_g;                                                                                          \
		while(part_fnc(_g) < (part_id))                                                                        \
			++_g;                                                                                          \
		_g;                                                                                                    \
	})

/**
 * @brief Initialize the global data structures for the LPs
 */
void lp_global_init(void)
{
	lid_node_first = partition_start(nid, n_nodes, lid_to_nid, 0, global_config.lps);
	n_lps_node = partition_start(nid + 1, n_nodes, lid_to_nid, 0, global_config.lps) - lid_node_first;

	lps = mm_alloc(sizeof(*lps) * n_lps_node);
	lps -= lid_node_first;

	if(n_lps_node < global_config.n_threads) {
		logger(LOG_WARN, "The simulation will run with %u threads instead of the requested %u", n_lps_node,
		    global_config.n_threads);
		global_config.n_threads = n_lps_node;
	}
}

/**
 * @brief Finalize the global data structures for the LPs
 */
void lp_global_fini(void)
{
	lps += lid_node_first;
	mm_free(lps);
}

/**
 * @brief Initialize the data structures of the LPs hosted in the calling thread
 */
void lp_init(void)
{
	lid_thread_first = partition_start(rid, global_config.n_threads, lid_to_rid, lid_node_first, n_lps_node);
	lid_thread_end = partition_start(rid + 1, global_config.n_threads, lid_to_rid, lid_node_first, n_lps_node);

	for(uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		struct lp_ctx *lp = &lps[i];
		current_lp = lp;

		model_allocator_lp_init(&lp->mm_state);
		lp->lib_ctx = rs_malloc(sizeof(*lp->lib_ctx));

		msg_queue_lp_init();
		lib_lp_init();
		auto_ckpt_lp_init(&lp->auto_ckpt);
		process_lp_init();
		termination_lp_init();
	}
}

/**
 * @brief Finalize the data structures of the LPs hosted in the calling thread
 */
void lp_fini(void)
{
	if(sync_thread_barrier()) {
		for(uint64_t i = 0; i < n_lps_node; ++i) {
			current_lp = &lps[i + lid_node_first];
			process_lp_deinit();
		}
	}

	sync_thread_barrier();

	for(uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		struct lp_ctx *lp = &lps[i];
		current_lp = lp;

		process_lp_fini();
		lib_lp_fini();
		msg_queue_lp_fini();
		model_allocator_lp_fini(&lp->mm_state);
	}

	current_lp = NULL;
}

/**
 * @brief Do housekeeping operations on the thread-locally hosted LPs after a fresh GVT
 * @param gvt the value of the freshly computed GVT
 */
void lp_on_gvt(simtime_t gvt)
{
	for(uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		struct lp_ctx *lp = &lps[i];
		fossil_lp_on_gvt(lp, gvt);
		auto_ckpt_lp_on_gvt(&lp->auto_ckpt, model_allocator_state_size(&lp->mm_state));
	}
}

/**
 * @brief Compute the id of the currently processed LP
 * @return the id of the current LP
 */
lp_id_t lp_id_get(void)
{
	return current_lp - lps;
}

/**
 * @brief Retrieve the user libraries dynamic state of the current LP
 * @return a pointer to the user libraries dynamic state of the current LP
 */
struct lib_ctx *lib_ctx_get(void)
{
	return current_lp->lib_ctx;
}
