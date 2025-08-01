/**
 * @file distributed/control_msg.h
 *
 * @brief MPI remote control messages header
 *
 * The module in which remote control messages are wired to the other modules
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

/// A control message MPI tag value
enum control_msg_type {
	/// Used by the master to start a new gvt reduction operation
	MSG_CTRL_GVT_START = 1,
	/// Used by slaves to signal their completion of the gvt protocol
	MSG_CTRL_GVT_DONE,
	/// Used in broadcast to signal that local LPs can terminate
	MSG_CTRL_LP_END_TERMINATION,
	/// Used in broadcast to signal that local LPs can terminate
	MSG_CTRL_PHASE_ORANGE_TERMINATION,
	/// Used in broadcast to signal that local LPs can terminate
	MSG_CTRL_PHASE_PURPLE_TERMINATION
};

extern void control_msg_process(enum control_msg_type ctrl);
