#include <serial/serial.h>
#include <stdlib.h>

#include <core/init.h>
#include <datatypes/heap.h>
#include <lp/message.h>
#include <mm/msg_allocator.h>
#include <lib/lib.h>

#define cmp_msgs(a, b) (a->destination_time < b->destination_time)

static binary_heap(lp_msg *) queue;
static uint64_t terminating_total;
static struct serial_lp *lp_serial;

struct serial_lp *current_lp;
unsigned n_prc_tot;

static void serial_simulation_init(void)
{
	n_prc_tot = global_config.lps_cnt;
	lp_serial = malloc(sizeof(*lp_serial) * global_config.lps_cnt);
	memset(lp_serial, 0, sizeof(*lp_serial) * global_config.lps_cnt);
	msg_allocator_init();
	heap_init(queue);

	for(uint64_t i = 0; i < global_config.lps_cnt; ++i){
		lib_init(&(lp_serial[i]->lib_state), i);
#if LOG_DEBUG >= LOG_LEVEL
		lp_serial[i].last_evt_time = -1;
#endif
		ScheduleNewEvent(i, 0, 0, NULL, 0);
	}
}

static void serial_simulation_fini(void)
{
	for(unsigned i = 0; i < heap_count(queue); ++i){
		msg_free(heap_items(queue)[i]);
	}

	for(uint64_t i = 0; i < global_config.lps_cnt; ++i){
		ProcessEvent(i, 0, UINT_MAX, NULL, 0, lp_serial[i].user_state);
		lib_fini(&(lp_serial[i]->lib_state));
	}

	heap_fini(queue);
	msg_allocator_fini();
	free(lp_serial);
}

void serial_simulation_run(void)
{
	while(likely(!heap_is_empty(queue))) {
		const lp_msg *current_msg = heap_min(queue);
		current_lp = &lp_serial[current_msg->destination];

#if LOG_DEBUG >= LOG_LEVEL
		if(log_is_lvl(LOG_DEBUG)) {
			if(current_msg->destination_time == current_lp->last_evt_time)
				log_log(
					LOG_DEBUG,
					"LP %u got two consecutive events with same timestamp %lf",
					current_msg->destination,
					current_msg->destination_time
				);
			current_lp->last_evt_time = current_msg->destination_time;
		}
#endif

		ProcessEvent(
			current_msg->destination,
			current_msg->destination_time,
			*((unsigned *) current_msg->payload),
			&current_msg->payload[sizeof(unsigned)],
			current_msg->payload_size - sizeof(unsigned),
			current_lp->user_state
		);

		bool onGVTres = OnGVT(
			current_msg->destination, current_lp->user_state);

		if(onGVTres != current_lp->terminating) {
			current_lp->terminating = onGVTres;
			terminating_total += ((int)onGVTres * 2) - 1;

			if(unlikely(onGVTres &&
				terminating_total >= global_config.lps_cnt)) {
				break;
			}
		}
		msg_free(heap_extract(queue, cmp_msgs));
	}
}

void ScheduleNewEvent(unsigned receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size)
{
	lp_msg *msg = msg_alloc(payload_size + sizeof(unsigned));
	msg->destination = receiver;
	msg->destination_time = timestamp;
	*((unsigned *) msg->payload) = event_type;
	memcpy(&msg->payload[sizeof(unsigned)], payload, payload_size);

	heap_insert(queue, cmp_msgs, msg);
}

void SetState(void *state)
{
	current_lp->user_state = state;
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);
	log_log(LOG_INFO, "Initializing serial simulation");
	serial_simulation_init();
	log_log(LOG_INFO, "Starting simulation");
	serial_simulation_run();
	log_log(LOG_INFO, "Finalizing simulation");
	serial_simulation_fini();
}
