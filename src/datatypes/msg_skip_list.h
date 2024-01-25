#pragma once

#include <datatypes/array.h>
#include <lp/msg.h>

/// We have 4^12
#define MSG_SKIP_LIST_MAX_HEIGHT 12

struct msg_skip_list_node {
	array_count_t next[MSG_SKIP_LIST_MAX_HEIGHT];
	simtime_t msg_t;
	struct lp_msg *msg;
};

struct msg_skip_list {
	struct msg_skip_list_node *nodes;
	array_count_t capacity;
	array_count_t free_list;
	array_count_t next[MSG_SKIP_LIST_MAX_HEIGHT];
};

extern void msg_skip_list_init(struct msg_skip_list *msl);
extern void msg_skip_list_fini(struct msg_skip_list *msl);

extern void msg_skip_list_insert(struct msg_skip_list *msl, struct lp_msg *msg);
extern struct lp_msg *msg_skip_list_try_anti(struct msg_skip_list *msl, const struct lp_msg *msg);
extern struct lp_msg *msg_skip_list_extract(struct msg_skip_list *msl);
