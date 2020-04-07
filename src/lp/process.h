#pragma once

#include <datatypes/array.h>
#include <lp/msg.h>
#include <mm/model_allocator.h>

struct process_data {
	dyn_array(lp_msg *) past_msgs;	//!< todo documentation
	dyn_array(lp_msg *) sent_msgs;	//!< todo documentation
	dyn_array(struct log_entry {
		array_count_t i_past_msg;
		mm_checkpoint *chkp;
	}) logs;			//!< todo documentation
};

#define process_is_silent() (current_lp->state == LP_STATE_SILENT)

extern void process_init(void);
extern void process_fini(void);

extern void process_msg(void);
extern void process_msg_sent(lp_msg *msg);
