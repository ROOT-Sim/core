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
	struct lib_state_managed lsm;
	struct mm_state mm_state;		//!< The memory allocator state for this LP
};

typedef struct _lp_struct lp_struct;

extern __thread lp_struct *current_lp;
extern lp_struct *lps;
extern unsigned *lid_to_rid;

extern void lp_global_init(void);
extern void lp_global_fini(void);

extern void lp_init(void);
extern void lp_fini(void);
