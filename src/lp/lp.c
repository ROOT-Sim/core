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

__thread uint64_t lp_id_first;
__thread uint64_t lp_id_end;

__thread struct lp_ctx *current_lp;
struct lp_ctx *lps;
lp_id_t n_lps_node;

#define lp_start_id_compute(trd)					\
__extension__({								\
	lp_id_t _g = (trd) * n_lps_node / n_threads;			\
	while (_g && lid_to_rid(_g) >= (trd))				\
		--_g;							\
	while (lid_to_rid(_g) < (trd))					\
		++_g;							\
	_g;								\
})

static void lp_iterator_init(void)
{
	lp_id_first = lp_start_id_compute(rid);
	lp_id_end = lp_start_id_compute(rid + 1);
}

void lp_global_init(void)
{
	uint64_t lps_offset = nid * n_lps / n_nodes;
	n_lps_node = ((nid + 1) * n_lps) / n_nodes - lps_offset;

	lps = mm_alloc(sizeof(*lps) * n_lps_node);
	lps -= lps_offset;

	if (n_lps_node < n_threads) {
		log_log(
			LOG_WARN,
			"The simulation will run with %u threads instead of the available %u",
			n_lps_node,
			n_threads
		);
		n_threads = n_lps_node;
	}
}

void lp_global_fini(void)
{
	uint64_t lps_offset = nid * n_lps / n_nodes;
	lps += lps_offset;

	mm_free(lps);
}

void lp_init(void)
{
	lp_iterator_init();

	for (uint64_t i = lp_id_first; i < lp_id_end; ++i) {
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
		uint64_t lps_offset = nid * n_lps / n_nodes;
		for (uint64_t i = 0; i < n_lps_node ; ++i) {
			current_lp = &lps[i + lps_offset];
			process_lp_deinit();
		}
	}

	sync_thread_barrier();

	for (uint64_t i = lp_id_first; i < lp_id_end; ++i) {
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
