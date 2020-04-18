#pragma once

#include <stdatomic.h>

extern atomic_uint nodes_to_end;

#define termination_cant_end() atomic_load_explicit(&nodes_to_end, memory_order_relaxed)

extern void termination_global_init(void);
extern void termination_lp_init(void);
extern void termination_on_msg_process(void);
extern void termination_on_gvt(void);
extern void termination_on_lp_rollback(void);
