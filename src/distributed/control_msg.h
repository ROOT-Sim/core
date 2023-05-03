/**
 * @file distributed/control_msg.h
 *
 * @brief MPI remote control messages header
 *
 * The module in which remote control messages are wired to the other modules
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <gvt/gvt.h>
#include <gvt/termination.h>

/// A control message MPI tag value
enum msg_ctrl_code {
	/// Used by the master to start a new gvt reduction operation
	MSG_CTRL_GVT_START = 1,
	/// Used by slaves to signal their completion of the gvt protocol
	MSG_CTRL_GVT_DONE,
	/// Used in broadcast to signal that local LPs can terminate
	MSG_CTRL_TERMINATION
};

extern void control_msg_process(enum msg_ctrl_code ctrl);
