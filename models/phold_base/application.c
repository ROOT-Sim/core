#include "ROOT-Sim.h"

void ProcessEvent(unsigned me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, unsigned *state){
	if(event_type == INIT){
		unsigned *new_state = malloc(sizeof(unsigned));
		*new_state = 0;
		SetState(new_state);
	} else {
		++*state;
	}

	ScheduleNewEvent(Random() * n_lps, now + Random() * 5.0, 1, NULL, 0);
}

bool OnGVT(unsigned me, const unsigned *state){
	return *state > 1000;
}
