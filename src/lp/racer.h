#pragma once

#include <lp/msg.h>

extern void racer_init(void);
extern void racer_fini(void);
extern bool racer_handle(struct lp_msg *msg);
extern bool racer_phase(void);
