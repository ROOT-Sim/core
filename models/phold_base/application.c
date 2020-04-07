#include "ROOT-Sim.h"

void ProcessEvent(unsigned me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, unsigned *state){
	switch (event_type) {
	case DEINIT:
		free(state);
		return;
	case INIT:
		state = malloc(sizeof(unsigned));
		*state = 0;
		SetState(state);
		break;
	default:
		++*state;
		break;
	}

	ScheduleNewEvent(Random() * n_lps, now + Random() * 5.0, 1, NULL, 0);
}

bool CanEnd(unsigned me, const unsigned *state){
	return *state > 100;
}
