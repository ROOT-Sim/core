#pragma once

#include <core/core.h>
#include <lp/msg.h>

extern void msg_queue_global_init(void);
extern void msg_queue_global_fini(void);
extern void msg_queue_init	(void);
extern void msg_queue_fini	(void);
extern void msg_queue_extract	(void);
extern simtime_t msg_queue_time_peek(void);
extern void msg_queue_insert	(lp_msg *msg);
