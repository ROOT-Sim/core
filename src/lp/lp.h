#pragma once

#include <core/core.h>
#include <lp/message.h>

#include <mm/model_allocator.h>
#include <datatypes/array.h>

enum lp_state {
	LP_RUNNING,
	LP_STOPPED
};

struct _lp_id { // BIG TODO: study layout id
	unsigned node_id : 16;
	unsigned x : 16;
	unsigned y : 16;
	unsigned z : 16;
};

struct _lp_struct {
	lp_id_t id;			//!< The LP unique id in the simulation
	void *user_state;		//!< A pointer to the user status (changeable with a call to SetState())
	enum lp_state state;		//!< The LP state
	dyn_array(lp_id_t) destinations;//!< todo documentation
	dyn_array(lp_msg *) past_msgs;	//!< todo documentation
	dyn_array(lp_msg *) sent_msgs;	//!< todo documentation
	dyn_array(mm_checkpoint *) logs;//!< todo documentation
	mm_state mm_state;		//!< The memory allocator state for this LP
};

typedef struct _lp_struct lp_struct;

extern __thread lp_struct *current_lp;
extern lp_struct *lps;
extern uint64_t lps_count;

extern void lp_global_init(uint64_t lp_cnt);
extern void lp_global_fini(void);
