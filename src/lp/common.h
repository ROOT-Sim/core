/**
 * @file lp/common.h
 *
 * @brief Common LP and message functionalities
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

/**
 * @brief Process a message for a given LP (Logical Process)
 *
 * This function handles the processing of a message by invoking the dispatcher and recording relevant statistics about
 * the processing time and count.
 *
 * @param lp A pointer to the LP associated with the message.
 * @param msg A pointer to the message to be processed.
 */
static inline void common_msg_process(const struct lp_ctx *lp, const struct lp_msg *msg)
{
	timer_uint t = timer_hr_new();
#ifndef NDEBUG
	current_msg = msg;
#endif
	global_config.dispatcher(msg->dest, msg->dest_t, msg->m_type, msg->pl, msg->pl_size, lp->state_pointer);
#ifndef NDEBUG
	current_msg = NULL;
#endif
	stats_take(STATS_MSG_PROCESSED_TIME, timer_hr_value(t));
	stats_take(STATS_MSG_PROCESSED, 1);
}

/**
 * @brief Allocate and populate a new message
 *
 * This function allocates a new message with the specified payload size and populates its fields with the provided
 * parameters. If a payload is provided, it is copied into the message's payload buffer.
 *
 * @param receiver The ID of the LP that will receive this message.
 * @param timestamp The logical time at which this message must be processed.
 * @param event_type A field used by the model to distinguish event types.
 * @param payload A pointer to the payload data to copy into the message.
 * @param payload_size The size in bytes of the payload to copy into the message.
 * @return A pointer to the newly allocated and populated message.
 */
static inline struct lp_msg *common_msg_pack(const lp_id_t receiver, const simtime_t timestamp,
    const unsigned event_type, const void *payload, const unsigned payload_size)
{
	struct lp_msg *msg = msg_allocator_alloc(payload_size);

	msg->dest = receiver;
	msg->dest_t = timestamp;
	msg->m_type = event_type;
	msg->termination_flags = 0;

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
	} else {
		msg->send_t = -1.0;
	}
#endif
	return msg;
}
