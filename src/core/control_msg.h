/**
 * @file core/control_msg.h
 *
 * @brief MPI remote control messages header
 *
 * The module in which remote control messages are wired to the other modules
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include "lp/msg.h"
#include "gvt/gvt.h"
#include "gvt/termination.h"
#include "include/ROOT-Sim/sdk.h"

/// The initial size of the handlers dynamic vector
#define INITIAL_HANDLERS_CAPACITY 4
/// The first control message ID that will be returned to the external libraries
#define FIRST_LIBRARY_CONTROL_MSG_ID 0

/// The size of the maximum payload of a control message
#define CONTROL_MSG_PAYLOAD_SIZE 16

/// A control message MPI tag value
enum platform_ctrl_msg_code {
	/// Used by the master to start a new gvt reduction operation
	MSG_CTRL_GVT_START = 1,
	/// Used by slaves to signal their completion of the gvt protocol
	MSG_CTRL_GVT_DONE,
	/// Used in broadcast to signal that local LPs can terminate
	MSG_CTRL_TERMINATION
};

/// A control message that can be user by higher-level libraries to synchronize actions
/* cppcheck-suppress misra-c2012-2.4
 * The payload is anything used specified, and the size is checked in the code */
struct library_ctrl_msg {
	/// The control message code
	unsigned ctrl_code;
	/// The payload of the control message
	unsigned char payload[CONTROL_MSG_PAYLOAD_SIZE];
};

extern void control_msg_init(void);
extern void control_msg_fini(void);
extern void control_msg_process(enum platform_ctrl_msg_code ctrl);
extern void invoke_library_handler(unsigned code, const void *payload);
