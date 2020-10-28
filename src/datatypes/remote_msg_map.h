#pragma once

#include <lp/msg.h>

extern void remote_msg_map_global_init(void);
extern void remote_msg_map_global_fini(void);
extern void remote_msg_map_fossil_collect(simtime_t current_gvt);
extern void remote_msg_map_match(uintptr_t msg_id, nid_t nid, lp_msg *msg);
