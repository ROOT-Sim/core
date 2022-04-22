/**
 * @file serial/serial.c
 *
 * @brief Sequential simulation engine
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdlib.h>

#include <serial/serial.h>
#include <arch/timer.h>
#include <core/core.h>
#include <datatypes/heap.h>
#include <lib/lib.h>
#include <log/stats.h>
#include <lp/msg.h>
#include <mm/msg_allocator.h>
#include <lp/lp.h>


/// The messages queue of the serial runtime
static heap_declare(struct lp_msg *) queue;
#if LOG_DEBUG >= LOG_LEVEL
/// Used for debugging possibly inconsistent models
static simtime_t current_evt_time;
#endif

void ScheduleNewEvent_serial(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload,
    unsigned payload_size);

/**
 * @brief Initialize the serial simulation environment
 */
static void serial_simulation_init(void)
{
	stats_global_init();
	stats_init();
	msg_allocator_init();
	heap_init(queue);
	lib_global_init();

	ScheduleNewEvent = ScheduleNewEvent_serial;

	lps = mm_alloc(sizeof(*lps) * global_config.lps);
	memset(lps, 0, sizeof(*lps) * global_config.lps);

	for(uint64_t i = 0; i < global_config.lps; ++i) {
		current_lp = &lps[i];
		model_allocator_lp_init();
		current_lp->lib_ctx = rs_malloc(sizeof(*current_lp->lib_ctx));
		lib_lp_init();
		lps[i].last_evt_time = -1;
		global_config.dispatcher(i, 0, LP_INIT, NULL, 0, lps[i].lib_ctx->state_s);
	}
}

/**
 * @brief Finalizes the serial simulation environment
 */
static void serial_simulation_fini(void)
{
	for(uint64_t i = 0; i < global_config.lps; ++i) {
		current_lp = &lps[i];
		global_config.dispatcher(i, 0, LP_FINI, NULL, 0, lps[i].lib_ctx->state_s);
		lib_lp_fini();
	}

	for(array_count_t i = 0; i < array_count(queue); ++i) {
		msg_allocator_free(array_get_at(queue, i));
	}

	lib_global_fini();
	heap_fini(queue);
	msg_allocator_fini();
	stats_global_fini();
}

/**
 * @brief Runs the serial simulation
 */
static int serial_simulation_run(void)
{
	timer_uint last_vt = timer_new();
	uint64_t to_terminate = global_config.lps;

	while(likely(!heap_is_empty(queue))) {
		const struct lp_msg *cur_msg = heap_min(queue);
		struct lp_ctx *this_lp = &lps[cur_msg->dest];
		current_lp = this_lp;

		if(cur_msg->dest_t == current_lp->last_evt_time)
			logger(LOG_DEBUG, "LP %u got two consecutive events with same timestamp %lf", cur_msg->dest,
			    cur_msg->dest_t);
		current_lp->last_evt_time = cur_msg->dest_t;
		current_evt_time = cur_msg->dest_t;

		global_config.dispatcher(cur_msg->dest, cur_msg->dest_t, cur_msg->m_type, cur_msg->pl, cur_msg->pl_size,
		    current_lp->lib_ctx->state_s);
		stats_take(STATS_MSG_PROCESSED, 1);

		bool can_end = global_config.committed(cur_msg->dest, current_lp->lib_ctx->state_s);

		if(can_end != current_lp->terminating) {
			current_lp->terminating = can_end;
			to_terminate += 1 - ((int)can_end * 2);

			if(unlikely(!to_terminate)) {
				stats_on_gvt(cur_msg->dest_t);
				break;
			}
		}

		if(global_config.gvt_period <= timer_value(last_vt)) {
			stats_on_gvt(cur_msg->dest_t);
			if(unlikely(cur_msg->dest_t >= global_config.termination_time))
				break;
			last_vt = timer_new();
		}

		msg_allocator_free(heap_extract(queue, msg_is_before));
	}

	stats_dump();

	return 0;
}

/**
 * @brief Schedule a new event. Sequential version.
 * @param receiver destination LP
 * @param timestamp timestamp of the injected event
 * @param event_type model-defined type
 * @param payload payload of the event
 * @param payload_size size of the payload
 */
void ScheduleNewEvent_serial(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload,
    unsigned payload_size)
{
	if(current_evt_time > timestamp)
		logger(LOG_WARN, "Sending a message in the PAST!");

	struct lp_msg *msg = msg_allocator_pack(receiver, timestamp, event_type, payload, payload_size);

	heap_insert(queue, msg_is_before, msg);
}

/**
 * @brief Handles a full serial simulation runs
 */
int serial_simulation(void)
{
	int ret;

	logger(LOG_INFO, "Initializing serial simulation");
	serial_simulation_init();
	stats_global_time_take(STATS_GLOBAL_INIT_END);

	stats_global_time_take(STATS_GLOBAL_EVENTS_START);
	logger(LOG_INFO, "Starting simulation");
	ret = serial_simulation_run();
	stats_global_time_take(STATS_GLOBAL_EVENTS_END);

	stats_global_time_take(STATS_GLOBAL_FINI_START);
	logger(LOG_INFO, "Finalizing simulation");
	serial_simulation_fini();

	return ret;
}
