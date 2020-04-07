#include <lp/lp.h>

#include <core/sync.h>
#include <distributed/mpi.h>

__thread lp_struct *current_lp;
lp_struct *lps;
unsigned *lid_to_rid;

#define lps_offset_count(id, units_cnt, lps_cnt, out_offset, out_cnt)	\
{									\
	const unsigned __i = id;					\
	const unsigned __ucnt = units_cnt;				\
	const uint64_t __lpscnt = lps_cnt;				\
	const bool __big = __i >= (__ucnt - (__lpscnt % __ucnt));	\
									\
	out_offset = __big ?						\
		__lpscnt - ((__lpscnt / __ucnt + 1) * (__ucnt - __i)) : \
		((__lpscnt / __ucnt) * __i);				\
	out_cnt	= (__lpscnt / __ucnt) + __big;				\
}

void lp_global_init(void)
{
#ifdef HAVE_MPI
	uint64_t lps_offset, lps_cnt;
	lps_offset_count(nid, n_nodes, n_lps, lps_offset, lps_cnt);
#else
	uint64_t lps_cnt = n_lps;
#endif

	lps = mm_alloc(sizeof(*lps) * lps_cnt);
	lid_to_rid = mm_alloc(sizeof(*lid_to_rid) * lps_cnt);

#ifdef HAVE_MPI
	lps -= lps_offset;
	lid_to_rid -= lps_offset;
#endif
}

void lp_global_fini(void)
{

#ifdef HAVE_MPI
	uint64_t lps_offset, lps_cnt;
	lps_offset_count(nid, n_nodes, n_lps, lps_offset, lps_cnt);
	lps += lps_offset;
	lid_to_rid += lps_offset;
#endif

	mm_free(lps);
	mm_free(lid_to_rid);
}

void lp_init(void)
{
	uint64_t i, lps_cnt;
#ifdef HAVE_MPI
	lps_offset_count(nid, n_nodes, n_lps, i, lps_cnt);
	lps_offset_count(rid, n_threads, lps_cnt, i, lps_cnt);
#else
	lps_offset_count(rid, n_threads, n_lps, i, lps_cnt);
#endif

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
		lib_lp_init(i);
		process_lp_init();
		termination_lp_init();

		i++;
	}
}

void lp_fini(void)
{
	uint64_t i = 0, lps_cnt = n_lps;
#ifdef HAVE_MPI
	lps_offset_count(nid, n_nodes, n_lps, i, lps_cnt);
#endif
	sync_thread_barrier();

	if(!rid){
		for(uint64_t j = 0; j < lps_cnt; ++j){
			current_lp = &lps[j + i];
			process_lp_deinit();
		}
	}

	sync_thread_barrier();

	lps_offset_count(rid, n_threads, lps_cnt, i, lps_cnt);

	while(lps_cnt--){
		current_lp = &lps[i];

		process_lp_fini();
		lib_lp_fini();
		model_memory_lp_fini();

		i++;
	}
}
