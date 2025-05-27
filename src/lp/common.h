/**
* @file lp/common.h
*
* @brief Common LP functionalities
*
* SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <arch/timer.h>
#include <lp/lp.h>
#include <lp/msg.h>
#include <log/log.h>
#include <log/stats.h>

/**
 * @brief Processes a message for a given LP (Logical Process).
 *
 * This function handles the processing of a message by invoking the dispatcher
 * and recording relevant statistics about the processing time and count.
 *
 * @param lp A pointer to the LP associated with the message.
 * @param msg A pointer to the message to be processed.
 */
static inline void common_msg_process(const struct lp_ctx *lp, const struct lp_msg *msg)
{
	timer_uint t = timer_hr_new();
	global_config.dispatcher(msg->dest, msg->dest_t, msg->m_type, msg->pl, msg->pl_size, lp->state_pointer);
	stats_take(STATS_MSG_PROCESSED_TIME, timer_hr_value(t));
	stats_take(STATS_MSG_PROCESSED, 1);
}
