#pragma once

#include <core/core.h>

extern int main(int argc, char **argv);

extern void ScheduleNewEvent(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);

extern void SetState(void *state);
