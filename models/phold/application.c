#include <ROOT-Sim.h>

#include <math.h>

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

	lp_id_t dest = Random() * n_lps;
	ScheduleNewEvent(dest, now + fabs(Normal() * 5.0) + 20.0, 1, NULL, 0);
}

bool CanEnd(lp_id_t me, const unsigned *state)
{
	(void)me;
	return *state > 1000;
}
