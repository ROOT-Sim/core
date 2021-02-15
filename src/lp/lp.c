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

__thread struct lp_ctx *current_lp;
struct lp_ctx *lps;
rid_t *lid_to_rid;
lp_id_t n_lps_node;

void lp_global_init(void)
{
	n_lps_node = (n_lps + n_nodes - 1) / n_nodes;

	if(n_lps_node < n_threads){
		log_log(
			LOG_WARN,
			"The simulation will run with %u threads instead of the available %u",
			n_lps_node,
			n_threads
		);
		n_threads = n_lps_node;
	}

	lps = mm_alloc(sizeof(*lps) * n_lps_node);
	lid_to_rid = mm_alloc(sizeof(*lid_to_rid) * n_lps_node);

	uint64_t lps_offset = n_lps_node * nid;
	lps -= lps_offset;
	lid_to_rid -= lps_offset;
}

void lp_global_fini(void)
{
	uint64_t lps_offset = n_lps_node * nid;
	lps += lps_offset;
	lid_to_rid += lps_offset;

	mm_free(lps);
	mm_free(lid_to_rid);
}

void lp_init(void)
{
	uint64_t i, lps_cnt;

	lps_iter_init(i, lps_cnt);

	uint64_t i_b = i, lps_cnt_b = lps_cnt;
	while(lps_cnt_b--){
		lid_to_rid[i_b] = rid;

		i_b++;
	}

	sync_thread_barrier();

	while(lps_cnt--){
		current_lp = &lps[i];

		model_allocator_lp_init();

		current_lp->lib_ctx_p = malloc_mt(sizeof(*current_lp->lib_ctx_p));
		lib_lp_init_pr();
		process_lp_init();
		termination_lp_init();

		i++;
	}
}

void lp_fini(void)
{
	uint64_t lps_cnt = n_lps_node, i = n_lps_node * nid;

	if(nid + 1 == n_nodes)
		lps_cnt = n_lps - n_lps_node * (n_nodes - 1);

	sync_thread_barrier();

	if(!rid){
		for(uint64_t j = 0; j < lps_cnt; ++j){
			current_lp = &lps[j + i];
			process_lp_deinit();
		}
	}

	sync_thread_barrier();

	lps_iter_init(i, lps_cnt);

	while(lps_cnt--){
		current_lp = &lps[i];

		process_lp_fini();
		lib_lp_fini_pr();
		model_allocator_lp_fini();

		i++;
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
