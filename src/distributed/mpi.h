#pragma once

#include <lp/msg.h>

enum _msg_ctrl {
	MSG_CTRL_GVT = 1,
	MSG_CTRL_TERMINATION
};

extern void mpi_global_init(int *argc_p, char ***argv_p);
extern void mpi_global_fini(void);
extern void mpi_remote_msg_send(lp_msg *msg);
extern void mpi_control_msg_broadcast(enum _msg_ctrl ctrl);
extern void mpi_remote_msg_handle(void);
