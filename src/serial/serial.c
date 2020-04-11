#include <serial/serial.h>
#include <stdlib.h>

#include <core/core.h>
#include <core/init.h>
#include <log/stats.h>
#include <mm/msg_allocator.h>

struct serial_lp *cur_lp;

static binary_heap(lp_msg *) queue;
static struct serial_lp *lps;

static void serial_simulation_init(void)
{
	lps = mm_alloc(sizeof(*lps) * n_lps);
	memset(lps, 0, sizeof(*lps) * n_lps);
	msg_allocator_init();

	heap_init(queue);

	for(uint64_t i = 0; i < n_lps; ++i){
		cur_lp = &lps[i];
		lib_lp_init(i);
#if LOG_DEBUG >= LOG_LEVEL
		lps[i].last_evt_time = -1;
#endif

		ProcessEvent(i, 0, INIT, NULL, 0, lps[i].lsm.state_s);
	}
}

static void serial_simulation_fini(void)
{
	for(uint64_t i = 0; i < n_lps; ++i){
		cur_lp = &lps[i];
		ProcessEvent(i, 0, UINT_MAX, NULL, 0, lps[i].lsm.state_s);
		lib_lp_fini();
	}

	for(array_count_t i = 0; i < array_count(queue); ++i){
		msg_allocator_free(array_get_at(queue, i));
	}

	heap_fini(queue);
	msg_allocator_fini();
	mm_free(lps);
}

void serial_simulation_run(void)
{
	uint64_t to_terminate = n_lps;
	while(likely(!heap_is_empty(queue))) {
		const lp_msg *cur_msg = heap_min(queue);
		struct serial_lp *this_lp = &lps[cur_msg->dest];
		cur_lp = this_lp;

#if LOG_DEBUG >= LOG_LEVEL
		if(log_is_lvl(LOG_DEBUG)) {
			if(cur_msg->dest_t == cur_lp->last_evt_time)
				log_log(
					LOG_DEBUG,
					"LP %u got two consecutive events with same timestamp %lf",
					cur_msg->dest,
					cur_msg->dest_t
				);
			cur_lp->last_evt_time = cur_msg->dest_t;
		}
#endif

		stats_time_start(STATS_MSG_PROCESSED);

		ProcessEvent(
			cur_msg->dest,
			cur_msg->dest_t,
			cur_msg->m_type,
			cur_msg->pl,
			cur_msg->pl_size,
			cur_lp->lsm.state_s
		);

		bool can_end = CanEnd(cur_msg->dest, cur_lp->lsm.state_s);

		stats_time_take(STATS_MSG_PROCESSED);

		if(can_end != cur_lp->terminating) {
			cur_lp->terminating = can_end;
			to_terminate += 1 - ((int)can_end * 2);

			if(unlikely(can_end && !to_terminate)) {
				break;
			}
		}

		msg_allocator_free(heap_extract(queue, msg_is_before));
	}

	stats_dump();
}

void ScheduleNewEvent(unsigned receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size)
{
	lp_msg *msg = msg_allocator_pack(
		receiver, timestamp, event_type, payload, payload_size);
	heap_insert(queue, msg_is_before, msg);
}

int main(int argc, char **argv)
{
	init_args_parse(argc, argv);
	log_log(LOG_INFO, "Initializing serial simulation");
	serial_simulation_init();
	log_log(LOG_INFO, "Starting simulation");
	serial_simulation_run();
	log_log(LOG_INFO, "Finalizing simulation");
	serial_simulation_fini();
}
