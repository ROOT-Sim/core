#pragma once
#include <core/core.h>
#include <gvt/termination.h>
#include <lib/lib.h>
#include <lp/msg.h>
#include <lp/process.h>
#include <mm/model_allocator.h>

enum lp_state {
	LP_STATE_RUNNING,
	LP_STATE_SILENT,
	LP_STOPPED
};

struct _lp_struct {
	enum lp_state state;		//!< The LP state
	simtime_t t_d;
	struct process_data p;
	struct lib_state ls;
	struct lib_state_managed *lsm_p;
	struct mm_state mm_state;		//!< The memory allocator state for this LP
};

extern uint64_t n_lps_node;

#define lps_iter_init(start_i, lps_cnt)					\
{									\
	const unsigned __i = rid;					\
	const unsigned __ucnt = n_threads;				\
	const uint64_t __lpscnt = nid + 1 == n_nodes ? 			\
		n_lps - n_lps_node * (n_nodes - 1) : n_lps;		\
	const bool __big = __i >= (__ucnt - (__lpscnt % __ucnt));	\
									\
	start_i = __big ?						\
		__lpscnt - ((__lpscnt / __ucnt + 1) * (__ucnt - __i)) : \
		((__lpscnt / __ucnt) * __i);				\
	lps_cnt	= (__lpscnt / __ucnt) + __big;				\
}

typedef struct _lp_struct lp_struct;

extern __thread lp_struct *current_lp;
extern lp_struct *lps;
extern unsigned *lid_to_rid;

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);

extern void lp_cleanup(void);
