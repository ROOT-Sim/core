#include <lp/lp.h>

#include <core/sync.h>
#include <core/timer.h>
#include <gvt/fossil.h>

__thread lp_struct *current_lp;
lp_struct *lps;
unsigned *lid_to_rid;
uint64_t n_lps_node;

void lp_global_init(void)
{
	n_lps_node = n_lps / n_nodes;

	// does the last node get more or less LPs than the other ones?
	double ratio_a =
		((double)n_lps_node + 1) /
		(n_lps - (n_lps_node + 1)* (n_nodes - 1));
	double ratio_b =
		(n_lps - n_lps_node * (n_nodes - 1)) /
		((double)n_lps_node);

	n_lps_node += ratio_b > ratio_a;

	if(n_lps_node < n_threads){
		log_log(
			LOG_WARN,
			"The simulation will run with %u threads instead of %u",
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
		current_lp->state = LP_STATE_RUNNING;

		model_memory_lp_init();

		current_lp->lsm_p = __wrap_malloc(sizeof(*current_lp->lsm_p));
		lib_lp_init();
		process_lp_init();
		termination_lp_init();

		i++;
	}
}

void lp_fini(void)
{
	uint64_t lps_cnt = n_lps, i = n_lps * nid;

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
		lib_lp_fini();
		model_memory_lp_fini();

		i++;
	}

	current_lp = NULL;
}
