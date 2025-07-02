/**
* @file lp/common.c
*
* @brief Common LP and message functionalities
*
* SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <lp/common.h>

#include <serial/serial.h>

#ifndef NDEBUG
_Thread_local const struct lp_msg *current_msg;
#endif

void ScheduleNewEvent(const lp_id_t receiver, const simtime_t timestamp, const unsigned event_type,
    const void *payload, const unsigned payload_size)
{
#ifndef NDEBUG
	if(unlikely(event_type >= LP_INIT)) {
		logger(LOG_FATAL, "ScheduleNewEvent() is being called with an invalid event type!");
		abort();
	}
#endif
	if(unlikely(global_config.synchronization == SERIAL))
		ScheduleNewEvent_serial(receiver, timestamp, event_type, payload, payload_size);
	else
		ScheduleNewEvent_parallel(receiver, timestamp, event_type, payload, payload_size);
}

void SetState(void *state)
{
#ifndef NDEBUG
	if(unlikely(current_msg->m_type != LP_INIT)) {
		logger(LOG_FATAL, "SetState() is being called outside the LP_INIT event!");
		abort();
	}
#endif
	current_lp->state_pointer = state;
}
