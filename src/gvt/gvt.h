#pragma once

#include <core/core.h>

extern __thread simtime_t current_gvt;

extern void gvt_global_init	(void);
extern void gvt_init		(void);
extern void gvt_msg_processed	(simtime_t msg_timestamp);
extern void gvt_msg_sent	(simtime_t msg_timestamp);
extern void gvt_fini		(void);
extern void gvt_global_fini	(void);
