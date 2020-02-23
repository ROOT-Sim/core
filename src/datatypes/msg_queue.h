#pragma once

#include <lp/message.h>

extern void 	msg_queue_global_init	(unsigned threads_cnt);
extern void 	msg_queue_global_fini	(void);
extern lp_msg* 	msg_queue_extract	(void);
extern void 	msg_queue_insert	(lp_msg *msg);
