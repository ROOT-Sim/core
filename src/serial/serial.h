#pragma once

#include <core/core.h>
#include <datatypes/heap.h>
#include <lib/lib.h>
#include <lp/msg.h>

struct serial_lp {
	struct lib_state ls;
	struct lib_state_managed lsm;
#if LOG_DEBUG >= LOG_LEVEL
	simtime_t last_evt_time;
#endif
	bool terminating;
};

extern struct serial_lp *lps;
extern struct serial_lp *cur_lp;

extern int main(int argc, char **argv);

extern void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);
