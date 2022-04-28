/**
 * @file distributed/control_msg.c
 *
 * @brief MPI remote control messages module
 *
 * The module in which remote control messages are wired to the other modules
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <distributed/control_msg.h>

/**
 * @brief Handle a received control message
 * @param ctrl the tag of the received control message
 */
void control_msg_process(enum msg_ctrl_code ctrl)
{
	switch(ctrl) {
		case MSG_CTRL_GVT_START:
			gvt_start_processing();
			break;
		case MSG_CTRL_GVT_DONE:
			gvt_on_done_ctrl_msg();
			break;
		case MSG_CTRL_TERMINATION:
			termination_on_ctrl_msg();
			break;
		default:
			__builtin_unreachable();
	}
}
