#pragma once

#include <core/core.h>
#include <lp/msg.h>

extern void gvt_global_init	(void);
extern simtime_t gvt_msg_processed(void);
extern void gvt_on_start_msg	(void);
extern void gvt_on_done_msg	(void);

#ifdef NEUROME_MPI

extern __thread bool gvt_phase_green;
extern __thread unsigned remote_msg_sent[1 << MAX_NODES_BITS];
extern atomic_int remote_msg_received[2];

#define gvt_on_remote_msg_send(dest_nid)				\
__extension__({ remote_msg_sent[dest_nid]++; })

#define gvt_on_remote_msg_receive(msg_phase)				\
__extension__({ atomic_fetch_add_explicit(remote_msg_received + 	\
	msg_phase, 1U, memory_order_relaxed); })

#define gvt_phase_get() __extension__({ gvt_phase_green;})
#endif
