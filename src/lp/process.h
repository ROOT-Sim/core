#pragma once

#include <datatypes/array.h>
#include <gvt/gvt.h>
#include <lp/msg.h>

struct process_data {
	dyn_array(lp_msg *) past_msgs;	//!< todo documentation
	dyn_array(lp_msg *) sent_msgs;	//!< todo documentation
};

extern void process_lp_init(void);
extern void process_lp_deinit(void);
extern void process_lp_fini(void);

extern void process_msg(void);
extern void process_msg_sent(lp_msg *msg);
