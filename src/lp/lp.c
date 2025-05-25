/**
 * @file lp/lp.c
 *
 * @brief LP construction functions
 *
 * LP construction functions
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lp/lp.h>

#include <core/sync.h>

/// A pointer to the first LP bound to this thread
__thread struct lp_ctx *thread_first_lp;
/// A pointer to the currently processed LP context
__thread struct lp_ctx *current_lp;
/// A pointer to the LP contexts array
struct lp_ctx *lps;
/// The number of LPs hosted on this node
lp_id_t n_lps_node;

/**
 * @brief Compute the id of the node which hosts a given LP
 * @param lp_id the id of the LP
 * @return the id of the node which hosts the LP identified by @p lp_id
 */
#define lid_to_nid(lp_id) ((nid_t)((lp_id) * n_nodes / global_config.lps))

/**
 * @brief Compute the id of the thread which hosts a given LP
 * @param lp_id the id of the LP
 * @param lid_node_first the id of the first LP bound to this node
 * @return the id of the thread which hosts the LP identified by @p lp_id
 *
 * Horrible things may happen if @p lp_id is not locally hosted (use #lid_to_nid() to make sure of that!)
 */
#define lid_to_tid(lp_id, lid_node_first) ((tid_t)(((lp_id) - lid_node_first) * global_config.n_threads / n_lps_node))

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

	size_t lps_size = sizeof(*lps) * global_config.lps;
	size_t lps_pages = (lps_size + MEM_DETERMINISTIC_PAGE_SIZE - 1) / MEM_DETERMINISTIC_PAGE_SIZE;
	lps = mm_aligned_alloc(MEM_DETERMINISTIC_PAGE_SIZE, lps_pages * MEM_DETERMINISTIC_PAGE_SIZE);

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
	mm_aligned_free(lps);
}

/**
 * @brief Initialize the data structures of the LPs hosted in the calling thread
 */
void lp_init(void)
{
	lp_id_t lid_node_first = partition_start(nid, n_nodes, lid_to_nid, 0, global_config.lps);
	lp_id_t lid_node_last = partition_start(nid + 1, n_nodes, lid_to_nid, 0, global_config.lps);

	for(lp_id_t i = tid; i < global_config.lps; i += global_config.n_threads) {
		struct lp_ctx *lp = &lps[i];
		nid_t this_nid = lid_to_nid(i);

		if(this_nid != nid)
			lp->rid = LP_RID_FROM_NID(this_nid);
		else
			lp->rid = lid_to_tid(i, lid_node_first);

		lp->next = NULL;
	}

	sync_thread_barrier();

	struct lp_ctx **head = &thread_first_lp;
	for(lp_id_t i = lid_node_first; i < lid_node_last; ++i) {
		struct lp_ctx *lp = &lps[i];
		if(lp->rid != tid)
			continue;

		*head = lp;
		head = &lp->next;

		lp->state_pointer = NULL;
		lp->fossil_epoch = 0;
		lp->cost = 0;

		model_allocator_lp_init(&lp->mm);
		auto_ckpt_lp_init(&lp->auto_ckpt);
		process_lp_init(lp);
	}
	*head = NULL;
}

/**
 * @brief Finalize the data structures of the LPs hosted in the calling thread
 */
void lp_fini(void)
{
	for(struct lp_ctx *lp = thread_first_lp; lp; lp = lp->next) {
		process_lp_fini(lp);
		model_allocator_lp_fini(&lp->mm);
	}

	current_lp = NULL;
}
