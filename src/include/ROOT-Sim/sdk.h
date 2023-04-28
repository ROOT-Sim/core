/**
 * @file sdk.h
 *
 * @brief ROOT-Sim header for internal library development
 *
 * This header is intended to be used by ROOT-Sim library developers only.
 * It defines all the symbols which are needed to develop a library to be
 * used by simulation models.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdlib.h>

/// The size of the maximum payload of a control message
#define CONTROL_MSG_PAYLOAD_SIZE 16

// The type of control message handlers that libraries can register
typedef void (*control_msg_handler_t)(unsigned ctrl_msg_id, const void *payload);

/**
 * @brief Register a new control message handler
 *
 * This function registers a control message handler for internal library use.
 * When a handler is registered, a control message ID is automatically allocated
 * and returned to the caller.
 * The library function should then use that ID to send control messages to the
 * various MPI ranks. The size of the payloads of such control messages is fixed.
 *
 * @param handler the handler to register
 * @return the ID of the registered control message ID
 */
extern int control_msg_register_handler(control_msg_handler_t handler);

/**
 * @brief Send a control message to all simulation nodes.
 *
 * This function sends a control message to all nodes of the simulation.
 * The corresponding handler must be registered first via control_msg_register_handler().
 *
 * @param ctrl the control message ID, as returned by control_msg_register_handler()
 * @param payload the payload of the control message, must be of size CONTROL_MSG_PAYLOAD_SIZE at most
 */
extern void control_msg_broadcast(unsigned ctrl, const void *payload, size_t size);
