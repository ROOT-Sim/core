#pragma once

#include <lp/lp.h>
#include <lp/msg.h>

#define is_retractable(msg) ((msg)->m_type == LP_RETRACTABLE)

extern void retractable_lib_init(void);
extern void retractable_lib_lp_init(struct lp_ctx *lp_ctx);
extern void retractable_lib_fini(void);
extern void retractable_reschedule(const struct lp_ctx *lp_ctx);
extern struct lp_msg *retractable_extract(void);
extern simtime_t retractable_min_t(void);
extern bool retractable_is_before(simtime_t normal_t);
