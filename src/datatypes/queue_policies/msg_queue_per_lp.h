#pragma once

#include <lp/msg.h>

extern void msg_queue_per_lp_global_init(void);
extern void msg_queue_per_lp_global_fini(void);
extern void msg_queue_per_lp_init(void);
extern void msg_queue_per_lp_fini(void);
extern void msg_queue_per_lp_lp_init(void);
extern void msg_queue_per_lp_lp_fini(void);
extern struct lp_msg *msg_queue_per_lp_extract(void);
extern void msg_queue_per_lp_insert(struct lp_msg *msg);
extern simtime_t msg_queue_per_lp_time_peek(void);
