/**
 * @file lp/lp.c
 *
 * @brief LP construction functions
 *
 * LP construction functions
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lp/lp.h>

#include <core/sync.h>
#include <gvt/fossil.h>

uint64_t lid_node_first;
__thread uint64_t lid_thread_first;
__thread uint64_t lid_thread_end;

__thread struct lp_ctx *current_lp;
struct lp_ctx *lps;
lp_id_t n_lps_node;

#define lp_partition_start(part_id, part_fnc, lp_tot, part_cnt, offset)	\
__extension__({								\
	lp_id_t _g = (part_id) * (lp_tot) / (part_cnt) + offset;	\
	while (_g > offset && part_fnc(_g) >= (part_id))		\
		--_g;							\
	while (part_fnc(_g) < (part_id))				\
		++_g;							\
	_g;								\
})

void lp_global_init(void)
{
	lid_node_first = lp_partition_start(nid, lid_to_nid, n_lps, n_nodes, 0);
	n_lps_node = lp_partition_start(nid + 1, lid_to_nid, n_lps, n_nodes, 0)
			- lid_node_first;

	log_log(LOG_DEBUG, "n %lu %lu \n", lid_node_first, n_lps_node);

	lps = mm_alloc(sizeof(*lps) * n_lps_node);
	lps -= lid_node_first;

	if (n_lps_node < n_threads) {
		log_log(LOG_WARN, "The simulation will run with %u threads instead of the available %u",
				n_lps_node, n_threads);
		n_threads = n_lps_node;
	}
}

void lp_global_fini(void)
{
	lps += lid_node_first;
	mm_free(lps);
}

void lp_init(void)
{
	lid_thread_first = lp_partition_start(rid, lid_to_rid, n_lps_node,
			n_threads, lid_node_first);
	lid_thread_end = lp_partition_start(rid + 1, lid_to_rid, n_lps_node,
			n_threads, lid_node_first);

	log_log(LOG_DEBUG, "t %lu %lu %lu %u\n", lid_thread_first, lid_thread_end, n_lps_node, n_threads);

	for (uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		current_lp = &lps[i];

		model_allocator_lp_init();
		current_lp->lib_ctx_p = malloc_mt(sizeof(*current_lp->lib_ctx_p));
		lib_lp_init_pr();
		process_lp_init();
		termination_lp_init();
	}
}

void lp_fini(void)
{
	if (sync_thread_barrier()) {
		for (uint64_t i = 0; i < n_lps_node ; ++i) {
			current_lp = &lps[i + lid_node_first];
			process_lp_deinit();
		}
	}

	sync_thread_barrier();

	for (uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
		current_lp = &lps[i];

		process_lp_fini();
		lib_lp_fini_pr();
		model_allocator_lp_fini();
	}

	current_lp = NULL;
}

lp_id_t lp_id_get_mt(void)
{
	return current_lp - lps;
}

struct lib_ctx *lib_ctx_get_mt(void)
{
	return current_lp->lib_ctx_p;
}
