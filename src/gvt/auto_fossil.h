#pragma once

#include <core/core.h>

#define auto_fossil_is_needed(delta_t) ((delta_t) > auto_fossil_threshold)

extern __thread simtime_t auto_fossil_threshold;

extern void auto_fossil_init(void);
extern void auto_fossil_on_gvt(void);
