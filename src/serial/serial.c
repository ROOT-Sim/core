/**
* @file serial/serial.c
*
* @brief Sequential simlation engine
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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

/**
 * @brief Initializes the serial simulation environment
 */
static void serial_simulation_init(void)
{
	stats_global_init();
	stats_init();

	s_lps = mm_alloc(sizeof(*s_lps) * n_lps);
	memset(s_lps, 0, sizeof(*s_lps) * n_lps);

	msg_allocator_init();
	heap_init(queue);
	lib_global_init();

	for (uint64_t i = 0; i < n_lps; ++i) {
		s_current_lp = &s_lps[i];
		lib_lp_init();
#if LOG_DEBUG >= LOG_LEVEL
		s_lps[i].last_evt_time = -1;
#endif
		ProcessEvent(i, 0, INIT, NULL, 0, s_lps[i].lib_ctx.state_s);
	}
}

/**
 * @brief Finalizes the serial simulation environment
 */
static void serial_simulation_fini(void)
{
	for (uint64_t i = 0; i < n_lps; ++i) {
		s_current_lp = &s_lps[i];
		ProcessEvent(i, 0, UINT_MAX, NULL, 0, s_lps[i].lib_ctx.state_s);
		lib_lp_fini();
	}

	for (array_count_t i = 0; i < array_count(queue); ++i) {
		msg_allocator_free(array_get_at(queue, i));
	}

	lib_global_fini();
	heap_fini(queue);
	msg_allocator_fini();

	mm_free(s_lps);

	stats_fini();
	stats_global_fini();
}

/**
 * @brief Runs the serial simulation
 */
static void serial_simulation_run(void)
{
	timer_uint last_vt = timer_new();
	uint64_t to_terminate = n_lps;

	while(likely(!heap_is_empty(queue))) {
		const struct lp_msg *cur_msg = heap_min(queue);
		struct s_lp_ctx *this_lp = &s_lps[cur_msg->dest];
		s_current_lp = this_lp;

#if LOG_DEBUG >= LOG_LEVEL
		if(log_is_lvl(LOG_DEBUG)) {
			if(cur_msg->dest_t == s_current_lp->last_evt_time)
				log_log(
					LOG_DEBUG,
					"LP %u got two consecutive events with same timestamp %lf",
					cur_msg->dest,
					cur_msg->dest_t
				);
			s_current_lp->last_evt_time = cur_msg->dest_t;
		}
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
	log_log(LOG_INFO, "Starting simulation");
	serial_simulation_run();
	log_log(LOG_INFO, "Finalizing simulation");
	serial_simulation_fini();
}

void serial_termination_force(void)
{
	heap_count(queue) = 0;
}

lp_id_t lp_id_get(void)
{
	return s_current_lp - s_lps;
}

struct lib_ctx *lib_ctx_get(void)
{
	return &s_current_lp->lib_ctx;
}
