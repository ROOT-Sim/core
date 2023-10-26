/**
 * @file serial/serial.c
 *
 * @brief Sequential simulation engine
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <serial/serial.h>

#include <arch/timer.h>
#include <datatypes/heap.h>
#include <distributed/distributed_mem.h>
#include <log/stats.h>
#include <lp/common.h>

/// The messages queue of the serial runtime
static heap_declare(struct lp_msg *) queue;

/**
 * @brief Initialize the serial simulation environment
 */
static void serial_simulation_init(void)
{
	stats_global_init();
	distributed_mem_global_init();
	stats_init();
	msg_allocator_init();
	heap_init(queue);

	lps = mm_alloc(sizeof(*lps) * global_config.lps);
	memset(lps, 0, sizeof(*lps) * global_config.lps);

	n_lps_node = global_config.lps;

	for(lp_id_t i = 0; i < global_config.lps; ++i) {
		struct lp_ctx *lp = &lps[i];

		model_allocator_lp_init(&lp->mm_state);

		current_lp = lp;

		lp->state_pointer = NULL;

		struct lp_msg *msg = common_msg_pack(i, 0.0, LP_INIT, NULL, 0);
		heap_insert(queue, msg_is_before, msg);

		common_msg_process(lp, msg);

		msg_allocator_free(heap_extract(queue, msg_is_before));
	}
}

/**
 * @brief Finalizes the serial simulation environment
 */
static void serial_simulation_fini(void)
{
	for(lp_id_t i = 0; i < global_config.lps; ++i) {
		struct lp_ctx *lp = &lps[i];
		current_lp = lp;
		global_config.dispatcher(i, 0, LP_FINI, NULL, 0, lp->state_pointer);
		model_allocator_lp_fini(&lp->mm_state);
	}

	for(array_count_t i = 0; i < array_count(queue); ++i)
		msg_allocator_free(array_get_at(queue, i));

	mm_free(lps);

	heap_fini(queue);
	msg_allocator_fini();
	distributed_mem_global_fini();
	stats_global_fini();
}

/**
 * @brief Runs the serial simulation
 */
static int serial_simulation_run(void)
{
	timer_uint last_vt = timer_new();

	while(likely(!heap_is_empty(queue))) {
		const struct lp_msg *msg = heap_min(queue);
		struct lp_ctx *lp = &lps[msg->dest];
		current_lp = lp;

		common_msg_process(lp, msg);

		if(global_config.gvt_period <= timer_value(last_vt)) {
			stats_on_gvt(msg->dest_t);
			if(unlikely(msg->dest_t >= global_config.termination_time))
				break;
			last_vt = timer_new();
		}

		timer_uint t = timer_hr_new();
		struct lp_msg *to_free = heap_extract(queue, msg_is_before);
		stats_take(STATS_MSG_EXTRACTION, timer_hr_value(t));
		msg_allocator_free(to_free);
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
	struct lp_msg *msg = common_msg_pack(receiver, timestamp, event_type, payload, payload_size);
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
