/**
 * @file lp/lp.c
 *
 * @brief LP construction functions
 *
 * LP construction functions
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lp/lp.h>

#include <core/sync.h>

/// A pointer to the currently processed LP context
__thread struct lp_ctx *current_lp;
/// A pointer to the LP contexts array
/** Valid entries are contained between #lid_node_first and #lid_node_first + #n_lps_node - 1, limits included */
struct lp_ctx *lps;
/// The number of LPs hosted on this node
lp_id_t n_lps_node;

#ifndef NDEBUG
_Thread_local bool lp_initialized;
#endif

/**
 * @brief Compute the id of the node which hosts a given LP
 * @param lp_id the id of the LP
 * @return the id of the node which hosts the LP identified by @p lp_id
 */
#define lid_to_nid(lp_id) ((nid_t)((lp_id) * n_nodes / global_config.lps))

/**
 * @brief Compute the id of the thread which hosts a given LP
 * @param lp_id the id of the LP
 * @return the id of the thread which hosts the LP identified by @p lp_id
 *
 * Horrible things may happen if @p lp_id is not locally hosted (use #lid_to_nid() to make sure of that!)
 */
#define lid_to_rid(lp_id) ((rid_t)(((lp_id) - lid_node_first) * global_config.n_threads / n_lps_node))

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
	lp_id_t lid_node_first = partition_start(nid, n_nodes, lid_to_nid, 0, global_config.lps);
	n_lps_node = partition_start(nid + 1, n_nodes, lid_to_nid, 0, global_config.lps) - lid_node_first;

	lps = mm_alloc(sizeof(*lps) * global_config.lps);

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
	mm_free(lps);
}

/**
 * @brief Initialize the data structures of the LPs hosted in the calling thread
 */
void lp_init(void)
{
	lp_id_t lid_node_first = partition_start(nid, n_nodes, lid_to_nid, 0, global_config.lps);
	lp_id_t lid_node_last = partition_start(nid + 1, n_nodes, lid_to_nid, 0, global_config.lps);

	for(lp_id_t i = rid; i < global_config.lps; i += global_config.n_threads) {
		struct lp_ctx *lp = &lps[i];

		nid_t this_nid = lid_to_nid(i);
		if(this_nid != nid) {
			lp->local = false;
			lp->id = this_nid;
		} else {
			lp->local = true;
			lp->id = lid_to_rid(i);
		}
	}

	sync_thread_barrier();

	for(lp_id_t i = lid_node_first; i < lid_node_last; ++i) {
		struct lp_ctx *lp = &lps[i];
		if(lp->id != rid)
			continue;

		model_allocator_lp_init(&lp->mm_state);
		lp->state_pointer = NULL;
		lp->fossil_epoch = 0;

		current_lp = lp;

		auto_ckpt_lp_init(&lp->auto_ckpt);
		process_lp_init(lp);
	}

	lp_initialized_set();
}

/**
 * @brief Finalize the data structures of the LPs hosted in the calling thread
 */
void lp_fini(void)
{
	for(lp_id_t i = rid; i < global_config.lps; i += global_config.n_threads) {
		struct lp_ctx *lp = &lps[i];

		if(!lp->local)
			continue;

		process_lp_fini(lp);
		model_allocator_lp_fini(&lp->mm_state);
	}

	current_lp = NULL;
}

/**
 * @brief Set the LP simulation state main pointer
 * @param state The state pointer to be passed to ProcessEvent() for the invoker LP
 */
void SetState(void *state)
{
#ifndef NDEBUG
	if(unlikely(lp_initialized)) {
		logger(LOG_FATAL, "SetState() is being called outside the LP_INIT event!");
		abort();
	}
#endif
	current_lp->state_pointer = state;
}
