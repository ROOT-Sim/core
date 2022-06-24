#pragma once

#include <ROOT-Sim.h>

#include <lp/lp.h>

struct lp_ctx;

extern void approximated_lp_on_rollback(void);
extern void approximated_lp_on_gvt(struct lp_ctx *ctx);
extern void approximated_lp_init(void);
