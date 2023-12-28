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
#include <mm/msg_allocator.h>

#ifndef NDEBUG
/// The currently processed message
/** This is not necessary for normal operation, but it's useful in debug */
extern __thread const struct lp_msg *current_msg;
#endif

static inline void common_msg_process(const struct lp_ctx *lp, const struct lp_msg *msg)
{
	timer_uint t = timer_hr_new();
#ifndef NDEBUG
	current_msg = msg;
#endif
	global_config.dispatcher(msg->dest, msg->dest_t, msg->m_type, msg->pl, msg->pl_size, lp->state_pointer);
	stats_take(STATS_MSG_PROCESSED_TIME, timer_hr_value(t));
	stats_take(STATS_MSG_PROCESSED, 1);
}

/**
 * @brief Allocate a new message and populate it
 * @param receiver the id of the LP which must receive this message
 * @param timestamp the logical time at which this message must be processed
 * @param event_type a field which can be used by the model to distinguish them
 * @param payload the payload to copy into the message
 * @param payload_size the size in bytes of the payload to copy into the message
 * @return a new populated message
 */
__attribute__((hot)) static inline struct lp_msg *common_msg_pack(lp_id_t receiver, simtime_t timestamp,
    unsigned event_type, const void *payload, unsigned payload_size)
{
	struct lp_msg *msg = msg_allocator_alloc(payload_size);

	msg->dest = receiver;
	msg->dest_t = timestamp;
	msg->m_type = event_type;

	if(likely(payload_size))
		memcpy(msg->pl, payload, payload_size);

#ifndef NDEBUG
	msg->send = current_lp - lps;
	if(likely(current_msg != NULL)) {
	       msg->send_t = current_msg->dest_t;
		if(unlikely(msg_is_before(msg, current_msg))) {
			logger(LOG_FATAL, "Scheduling a message in the past!");
			abort();
		}
	}
#endif
	return msg;
}
