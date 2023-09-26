#include <lp/common.h>

#include <serial/serial.h>

#ifndef NDEBUG
__thread const struct lp_msg *current_msg;
#endif

void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp, unsigned event_type, const void *payload,
    unsigned payload_size)
{
	if(unlikely(global_config.serial))
		ScheduleNewEvent_serial(receiver, timestamp, event_type, payload, payload_size);
	else
		ScheduleNewEvent_parallel(receiver, timestamp, event_type, payload, payload_size);
}
