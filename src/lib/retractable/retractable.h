#pragma once

#include <lp/msg.h>

#define is_retractable(msg) ((msg)->m_type == LP_RETRACTABLE)

extern void retractable_lib_init(void);
extern void retractable_lib_lp_init(void);
extern void retractable_lib_fini(void);

extern void retractable_rollback_handle(void);

extern struct lp_msg *retractable_extract(simtime_t normal_t);
extern simtime_t retractable_min_t(simtime_t normal_t);
