#pragma once

#include <core/core.h>
#include <lib/lib.h>

struct serial_lp {
	struct lib_state lib_state;
	void *user_state;
#if LOG_DEBUG >= LOG_LEVEL
	simtime_t last_evt_time;
#endif
	bool terminating;
};

extern struct serial_lp *current_lp;

extern int main(int argc, char **argv);

extern void ScheduleNewEvent(unsigned receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);

extern void SetState(void *state);
