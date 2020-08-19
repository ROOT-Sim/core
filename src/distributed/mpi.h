#pragma once

#ifdef NEUROME_MPI

#include <lp/msg.h>

enum _msg_ctrl {
	MSG_CTRL_GVT_START = 1,
	MSG_CTRL_GVT_DONE,
	MSG_CTRL_TERMINATION
};

extern void mpi_global_init(int *argc_p, char ***argv_p);
extern void mpi_global_fini(void);

extern void mpi_remote_msg_send(lp_msg *msg, nid_t dest_nid);
extern void mpi_remote_anti_msg_send(lp_msg *msg, nid_t dest_nid);

extern void mpi_control_msg_broadcast(enum _msg_ctrl ctrl);
extern void mpi_remote_msg_handle(void);

extern bool mpi_reduce_remote_sent_done(void);
extern void mpi_reduce_remote_sent(const uint32_t *sent_msgs, uint32_t *result);

extern bool mpi_reduce_local_min_done(void);
extern void mpi_reduce_local_min(simtime_t *local_min_p);

#endif
