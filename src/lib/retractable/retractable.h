#pragma once

#include <lp/lp.h>
#include <lp/msg.h>

#define is_retractable(msg) ((msg)->m_type == LP_RETRACTABLE)

extern void retractable_lib_init(void);
extern void retractable_lib_lp_init(struct lp_ctx *lp_ctx);
extern void retractable_lib_fini(void);
extern void retractable_reschedule(const struct lp_ctx *lp_ctx);
extern struct lp_msg *retractable_extract(void);
extern void retractable_post_silent(const struct lp_ctx *lp, simtime_t now);
extern bool retractable_is_before(simtime_t normal_t);
