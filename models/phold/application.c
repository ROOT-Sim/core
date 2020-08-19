#include "NeuRome.h"

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, unsigned *state)
{
	(void)me;
	(void)content;
	(void)size;
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

bool CanEnd(lp_id_t me, const unsigned *state)
{
	(void)me;
	return *state > 1000;
}
