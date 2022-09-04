#pragma once

#include <lp/msg.h>

extern void msg_queue_per_thread_global_init(void);
extern void msg_queue_per_thread_global_fini(void);
extern void msg_queue_per_thread_init(void);
extern void msg_queue_per_thread_fini(void);
extern void msg_queue_per_thread_lp_init(void);
extern void msg_queue_per_thread_lp_fini(void);
extern struct lp_msg *msg_queue_per_thread_extract(void);
extern void msg_queue_per_thread_insert(struct lp_msg *msg);
extern simtime_t msg_queue_per_thread_time_peek(void);
