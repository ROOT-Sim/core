/**
 * @file distributed/control_msg.h
 *
 * @brief MPI remote control messages header
 *
 * The module in which remote control messages are wired to the other modules
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <lp/msg.h>
#include <gvt/gvt.h>
#include <gvt/termination.h>
#include "include/ROOT-Sim/sdk.h"

#define INITIAL_HANDLERS_CAPACITY 4

/// A control message MPI tag value
enum default_msg_ctrl_code {
	/// Used by the master to start a new gvt reduction operation
	MSG_CTRL_GVT_START = 1,
	/// Used by slaves to signal their completion of the gvt protocol
	MSG_CTRL_GVT_DONE,
	/// Used in broadcast to signal that local LPs can terminate
	MSG_CTRL_TERMINATION,
	/// Marker to signal the end of the enum
	MSG_CTRL_DEFAULT_END
};

struct control_msg {
	/// The control message code
	unsigned ctrl_code;
	/// The payload of the control message
	char payload[CONTROL_MSG_PAYLOAD_SIZE];
};

_Static_assert(sizeof(struct control_msg) < msg_remote_anti_size(), "Invalid control message payload size");

extern void control_msg_process(unsigned ctrl, void *payload);
