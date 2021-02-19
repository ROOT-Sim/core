/**
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: CC0-1.0
 */
#include <ROOT-Sim.h>

#include <math.h>

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type,
	const void *content, unsigned size, unsigned *state)
{
	(void)me;
	(void)content;
	(void)size;
	switch (event_type) {
	case LP_INIT:
		state = malloc(sizeof(unsigned));
		*state = 0;
		SetState(state);
		break;
	case LP_FINI:
		free(state);
		return;
	case MODEL_INIT:
	case MODEL_FINI:
		return;
	default:
		++*state;
		break;
	}

	lp_id_t dest = Random() * n_lps;
	ScheduleNewEvent(dest, now + fabs(Normal() * 5.) + 20., MODEL_FINI + 1,
			NULL, 0);
}

bool CanEnd(lp_id_t me, const unsigned *state)
{
	(void)me;
	return *state > 1000;
}
