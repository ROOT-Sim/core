#include <lp/lp.h>

#include <distributed/mpi.h>

__thread lp_struct *current_lp;
lp_struct *lps;
unsigned *lid_to_rid;

static uint64_t n_lps_node;

static void lp_start_end(uint64_t *start_p, uint64_t *end_p)
{
	const unsigned this_t = rid;
	const unsigned thds_cnt = n_threads;
	const uint64_t lps_cnt = n_lps_node;
	const bool big = (this_t < (lps_cnt % thds_cnt));

	const uint64_t start = big ?
		lps_cnt - ((lps_cnt / thds_cnt + 1) * (thds_cnt - this_t)) :
		((lps_cnt / thds_cnt) * this_t);
	*start_p = start;
	*end_p = (lps_cnt / thds_cnt) + big + start;
}

void lp_global_init(void)
{
	uint64_t count = n_lps;

#ifdef HAVE_MPI
	count = (count / n_nodes) + 1 - (nid < (count % n_nodes));
#endif

	lps = malloc(sizeof(*lps) * count);
	lid_to_rid = malloc(sizeof(*lid_to_rid) * count);
	n_lps_node = count;
}

void lp_global_fini(void)
{
	free(lps);
	free(lid_to_rid);
}

void lp_init(void)
{
	uint64_t start, i;
	lp_start_end(&start, &i);

	while(i-- != start){
		current_lp = &lps[i];
		current_lp->state = LP_STATE_RUNNING;
		lid_to_rid[i] = rid;
		model_memory_lp_init();
		process_lp_init();
	}
}

void lp_fini(void)
{
	uint64_t start, i;
	lp_start_end(&start, &i);

	while(i-- != start){
		current_lp = &lps[i];

		process_lp_fini();
		model_memory_lp_fini();
	}
}
