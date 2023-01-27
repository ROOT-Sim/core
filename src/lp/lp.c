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

#ifndef NDEBUG
bool lp_initialized;
#endif

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
	lid_node_first = 0;
	n_lps_node = global_config.lps + global_config.lps_racer;

	lps = mm_alloc(sizeof(*lps) * n_lps_node);
	lps -= lid_node_first;
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
	if(rid < global_config.n_threads) {
		lid_thread_first =
		    partition_start(rid, global_config.n_threads, lid_to_rid, 0, global_config.lps);
		lid_thread_end =
		    partition_start(rid + 1, global_config.n_threads, lid_to_rid, 0, global_config.lps);
	} else {
		rid_t this_rid = rid - global_config.n_threads;
		lid_thread_first =
		    partition_start(this_rid, global_config.n_threads_racer, lid_to_rid, 0, global_config.lps_racer);
		lid_thread_end =
		    partition_start(this_rid + 1, global_config.n_threads_racer, lid_to_rid, 0, global_config.lps_racer);

		lid_thread_first += global_config.lps;
		lid_thread_end += global_config.lps;
	}
	for(uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		struct lp_ctx *lp = &lps[i];

		model_allocator_lp_init(&lp->mm_state);
		lp->state_pointer = NULL;
		lp->fossil_epoch = 0;

		current_lp = lp;
		lp->rng_ctx = rs_malloc(sizeof(*lp->rng_ctx));
		random_lib_lp_init(i, lp->rng_ctx);

		auto_ckpt_lp_init(&lp->auto_ckpt);
		process_lp_init(lp);
		termination_lp_init(lp);
	}
}

/**
 * @brief Finalize the data structures of the LPs hosted in the calling thread
 */
void lp_fini(void)
{
	for(uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		struct lp_ctx *lp = &lps[i];

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
