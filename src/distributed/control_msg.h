/**
 * @file distributed/control_msg.h
 *
 * @brief MPI remote control messages header
 *
 * The module in which remote control messages are wired to the other modules
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <gvt/gvt.h>
#include <gvt/termination.h>

/// A control message MPI tag value
enum msg_ctrl_tag {
	/// Used by the master to start a new gvt reduction operation
	MSG_CTRL_GVT_START,
	/// Used by slaves to signal their completion of the gvt protocol
	MSG_CTRL_GVT_DONE,
	/// Used in broadcast to signal that local LPs can terminate
	MSG_CTRL_TERMINATION
};

inline void control_msg_process(enum msg_ctrl_tag ctrl)
{
	switch (ctrl) {
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
