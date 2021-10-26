/**
 * @file serial/serial.c
 *
 * @brief Sequential simlation engine
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <serial/serial.h>

#include <arch/timer.h>
#include <core/core.h>
#include <core/init.h>
#include <datatypes/heap.h>
#include <lib/lib.h>
#include <log/stats.h>
#include <lp/msg.h>
#include <mm/msg_allocator.h>

#include <stdlib.h>

/// The LP context for the serial runtime
struct s_lp_ctx {
	/// The context for the model development libraries
	struct lib_ctx lib_ctx;
#if LOG_DEBUG >= LOG_LEVEL
	/// The logical time of the last processed event by this LP
	simtime_t last_evt_time;
#endif
	/// The last evaluation of the termination predicate for this LP
	bool terminating;
};

/// The array of all the simulation LP contexts
static struct s_lp_ctx *s_lps;
/// The context of the currently processed LP
static struct s_lp_ctx *s_current_lp;
/// The messages queue of the serial runtime
static binary_heap(struct lp_msg *) queue;
#if LOG_DEBUG >= LOG_LEVEL
/// Used for debugging possibly inconsistent models
static simtime_t current_evt_time;
#endif

void serial_model_init(void)
{
	struct s_lp_ctx tmp_lp = {0};
	s_current_lp = &tmp_lp;
	s_lps = s_current_lp - n_lps;

	lib_lp_init();
	ProcessEvent(0, 0, MODEL_INIT, NULL, 0, NULL);
	lib_lp_fini();
}

/**
 * @brief Initializes the serial simulation environment
 */
static void serial_simulation_init(void)
{
	stats_global_init();
	stats_init();
	msg_allocator_init();
	heap_init(queue);
	lib_global_init();

    // TODO: check if this works
	pubsub_module_global_init();

	serial_model_init();

	s_lps = mm_alloc(sizeof(*s_lps) * n_lps);
	memset(s_lps, 0, sizeof(*s_lps) * n_lps);

	for (uint64_t i = 0; i < n_lps; ++i) {
		s_current_lp = &s_lps[i];
		lib_lp_init();

        // TODO: check if this works
		pubsub_module_lp_init();

#if LOG_DEBUG >= LOG_LEVEL
		s_lps[i].last_evt_time = -1;
#endif
		ProcessEvent(i, 0, LP_INIT, NULL, 0, s_lps[i].lib_ctx.state_s);
	}
}

/**
 * @brief Finalizes the serial simulation environment
 */
static void serial_simulation_fini(void)
{
	for (uint64_t i = 0; i < n_lps; ++i) {
		s_current_lp = &s_lps[i];
		ProcessEvent(i, 0, LP_FINI, NULL, 0, s_lps[i].lib_ctx.state_s);
		lib_lp_fini();
	}

	ProcessEvent(0, 0, MODEL_FINI, NULL, 0, NULL);

	for (array_count_t i = 0; i < array_count(queue); ++i) {
		msg_allocator_free(array_get_at(queue, i));
	}

	mm_free(s_lps);

	lib_global_fini();
	heap_fini(queue);
	msg_allocator_fini();
	stats_global_fini();
}

/**
 * @brief Runs the serial simulation
 */
static void serial_simulation_run(void)
{
	timer_uint last_vt = timer_new();
	uint64_t to_terminate = n_lps;

	while (likely(!heap_is_empty(queue))) {
		const struct lp_msg *cur_msg = heap_min(queue);
		struct s_lp_ctx *this_lp = &s_lps[cur_msg->dest];
		s_current_lp = this_lp;

#if LOG_DEBUG >= LOG_LEVEL
		if (log_can_log(LOG_DEBUG)) {
			if(cur_msg->dest_t == s_current_lp->last_evt_time)
				log_log(
					LOG_DEBUG,
					"LP %u got two consecutive events with same timestamp %lf",
					cur_msg->dest,
					cur_msg->dest_t
				);
			s_current_lp->last_evt_time = cur_msg->dest_t;
		}
		current_evt_time = cur_msg->dest_t;
#endif

		stats_time_start(STATS_MSG_PROCESSED);

		ProcessEvent(
			cur_msg->dest,
			cur_msg->dest_t,
			cur_msg->m_type,
			cur_msg->pl,
			cur_msg->pl_size,
			s_current_lp->lib_ctx.state_s
		);

		stats_time_take(STATS_MSG_PROCESSED);

		bool can_end = CanEnd(cur_msg->dest, s_current_lp->lib_ctx.state_s);

		if (can_end != s_current_lp->terminating) {
			s_current_lp->terminating = can_end;
			to_terminate += 1 - ((int)can_end * 2);

			if (unlikely(!to_terminate)) {
				stats_on_gvt(cur_msg->dest_t);
				break;
			}
		}

		if (global_config.gvt_period <= timer_value(last_vt)) {
			stats_on_gvt(cur_msg->dest_t);
			if (unlikely(cur_msg->dest_t >=
				global_config.termination_time))
				break;
			last_vt = timer_new();
		}

		msg_allocator_free(heap_extract(queue, msg_is_before));
	}

	stats_dump();
}

void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size)
{
#if LOG_DEBUG >= LOG_LEVEL
	if (log_can_log(LOG_DEBUG) && current_evt_time > timestamp)
		log_log(LOG_DEBUG, "Sending a message in the PAST!");
#endif

	struct lp_msg *msg = msg_allocator_pack(
		receiver, timestamp, event_type, payload, payload_size);
	heap_insert(queue, msg_is_before, msg);
}

/**
 * @brief Handles a full serial simulation runs
 */
void serial_simulation(void)
{
	log_log(LOG_INFO, "Initializing serial simulation");
	serial_simulation_init();
	stats_global_time_take(STATS_GLOBAL_INIT_END);

	stats_global_time_take(STATS_GLOBAL_EVENTS_START);
	log_log(LOG_INFO, "Starting simulation");
	serial_simulation_run();
	stats_global_time_take(STATS_GLOBAL_EVENTS_END);

	stats_global_time_take(STATS_GLOBAL_FINI_START);
	log_log(LOG_INFO, "Finalizing simulation");
	serial_simulation_fini();
}

lp_id_t lp_id_get(void)
{
	return s_current_lp - s_lps;
}

struct lib_ctx *lib_ctx_get(void)
{
	return &s_current_lp->lib_ctx;
}
