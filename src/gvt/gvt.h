#pragma once

#include <core/core.h>

extern __thread simtime_t current_gvt;

extern void gvt_global_init	(void);
extern bool gvt_msg_processed	(void);
extern void gvt_msg_sent	(simtime_t msg_timestamp);
extern void gvt_global_fini	(void);
