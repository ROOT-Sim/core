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
extern void control_msg_process(enum msg_ctrl_tag ctrl);
