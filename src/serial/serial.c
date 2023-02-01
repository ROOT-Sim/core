/**
 * @file serial/serial.c
 *
 * @brief Sequential simulation engine
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <serial/serial.h>

#include <arch/timer.h>
#include <datatypes/heap.h>
#include <lib/random/random.h>
#include <log/stats.h>
#include <lp/common.h>
#include <mm/msg_allocator.h>

/// The messages queue of the serial runtime
static heap_declare(struct lp_msg *) queue;

/**
 * @brief Initialize the serial simulation environment
 */
static void serial_simulation_init(void)
{
	stats_global_init();
	stats_init();
	msg_allocator_init();
	heap_init(queue);

	lps = mm_alloc(sizeof(*lps) * global_config.lps);
	memset(lps, 0, sizeof(*lps) * global_config.lps);

	n_lps_node = global_config.lps;

	for(lp_id_t i = 0; i < global_config.lps; ++i) {
		struct lp_ctx *lp = &lps[i];

		lp->termination_t = -1;

		model_allocator_lp_init(&lp->mm_state);

		current_lp = lp;
		lp->rng_ctx = rs_malloc(sizeof(*lp->rng_ctx));
		random_lib_lp_init(i, lp->rng_ctx);

		lp->state_pointer = NULL;

		struct lp_msg *msg = msg_allocator_pack(i, 0.0, LP_INIT, NULL, 0);
		msg->raw_flags = 0;
		heap_insert(queue, msg_is_before, msg);

		common_msg_process(lp, msg);

		msg_allocator_free(heap_extract(queue, msg_is_before));
	}
	lp_initialized_set();
}

/**
 * @brief Finalizes the serial simulation environment
 */
static void serial_simulation_fini(void)
{
	for(uint64_t i = 0; i < global_config.lps; ++i) {
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
	stats_global_fini();
}

/**
 * @brief Runs the serial simulation
 */
static int serial_simulation_run(void)
{
	timer_uint last_vt = timer_new();
	lp_id_t to_terminate = global_config.lps;

	while(likely(!heap_is_empty(queue))) {
		const struct lp_msg *msg = heap_min(queue);
		struct lp_ctx *lp = &lps[msg->dest];
		current_lp = lp;

		common_msg_process(lp, msg);

		if(unlikely(lp->termination_t < 0 && global_config.committed(msg->dest, lp->state_pointer))) {
			lp->termination_t = msg->dest_t;
			if(unlikely(!--to_terminate)) {
				stats_on_gvt(msg->dest_t);
				break;
			}
		}

		if(global_config.gvt_period <= timer_value(last_vt)) {
			stats_on_gvt(msg->dest_t);
			if(unlikely(msg->dest_t >= global_config.termination_time))
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
	struct lp_msg *msg = msg_allocator_pack(receiver, timestamp, event_type, payload, payload_size);
	msg->raw_flags = 0;

#ifndef NDEBUG
	if(unlikely(msg_is_before(msg, heap_min(queue)))) {
		logger(LOG_FATAL, "Sending a message in the PAST!");
		abort();
	}
#endif

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
