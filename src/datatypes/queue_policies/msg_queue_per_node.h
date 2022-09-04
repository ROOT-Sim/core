#pragma once

#include <lp/msg.h>

extern void msg_queue_per_node_global_init(void);
extern void msg_queue_per_node_global_fini(void);
extern void msg_queue_per_node_init(void);
extern void msg_queue_per_node_fini(void);
extern void msg_queue_per_node_lp_init(void);
extern void msg_queue_per_node_lp_fini(void);
extern struct lp_msg *msg_queue_per_node_extract(void);
extern void msg_queue_per_node_insert(struct lp_msg *msg);
extern simtime_t msg_queue_per_node_time_peek(void);
