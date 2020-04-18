#pragma once

#include <core/core.h>
#include <lp/msg.h>

extern __thread simtime_t current_gvt;

extern void gvt_global_init	(void);
extern bool gvt_msg_processed	(void);
extern void gvt_remote_msg_tag	(lp_msg *msg);
extern void gvt_global_fini	(void);
