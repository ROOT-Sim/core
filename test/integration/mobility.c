/**
 * @file test/integration/mobility.c
 *
 * @brief Integration test with unstable predicate termination (Random Waypoint Mobility Model)
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <ROOT-Sim.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_rng.h"

#ifndef NUM_LPS
#define NUM_LPS 1024
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 0
#endif

#define AGENTS_PER_REGION 10
#define REGION_SIZE 1000.0

#define MIN_SPEED 1.0
#define MAX_SPEED 10.0
#define MIN_PAUSE 1.0
#define MAX_PAUSE 5.0

#define EV_PAUSE_END 1
#define EV_MOVE_END 2

struct agent_message {
	double x, y;
};

struct region_state {
	struct test_rng_state rng;
	int agents_in_region;
	int agents_paused;
};

static double uniform_range(struct test_rng_state *state, double a, double b)
{
	return a + (b - a) * test_rng_random(state);
}

void ProcessEvent(const lp_id_t me, const simtime_t now, const unsigned event_type, const void *content,
    const unsigned size, void *s)
{
	struct region_state *state = s;
	const struct agent_message *msg = content;
	const int grid_w = (int)sqrt(NUM_LPS);

	switch(event_type) {
		case LP_INIT:
			state = rs_malloc(sizeof(*state));
			if(state == NULL)
				abort();
			test_rng_set_seed(me, &state->rng);
			state->agents_in_region = 0;
			state->agents_paused = 0;
			SetState(state);

			for(int i = 0; i < AGENTS_PER_REGION; i++) {
				struct agent_message init_agent;
				init_agent.x =
				    (me % grid_w) * REGION_SIZE + uniform_range(&state->rng, 0.0, REGION_SIZE);
				init_agent.y =
				    (me / grid_w) * REGION_SIZE + uniform_range(&state->rng, 0.0, REGION_SIZE);

				state->agents_in_region++;
				state->agents_paused++;

				const double pause_time = uniform_range(&state->rng, MIN_PAUSE, MAX_PAUSE);
				ScheduleNewEvent(me, now + pause_time, EV_PAUSE_END, &init_agent, sizeof(init_agent));
			}
			break;

		case LP_FINI:
			break;

		case EV_PAUSE_END:
			{
				state->agents_paused--;
				state->agents_in_region--;

				const int my_x = me % grid_w;
				const int my_y = me / grid_w;

				int nx = my_x + (int)(uniform_range(&state->rng, 0.0, 3.0)) - 1;
				int ny = my_y + (int)(uniform_range(&state->rng, 0.0, 3.0)) - 1;

				if(nx < 0)
					nx = 0;
				if(nx >= grid_w)
					nx = grid_w - 1;
				if(ny < 0)
					ny = 0;
				if(ny >= grid_w)
					ny = grid_w - 1;

				lp_id_t dest_lp = ny * grid_w + nx;
				if(dest_lp >= NUM_LPS)
					dest_lp = me;

				const double dest_x = nx * REGION_SIZE + uniform_range(&state->rng, 0.0, REGION_SIZE);
				const double dest_y = ny * REGION_SIZE + uniform_range(&state->rng, 0.0, REGION_SIZE);

				const double speed = uniform_range(&state->rng, MIN_SPEED, MAX_SPEED);
				const double dx = dest_x - msg->x;
				const double dy = dest_y - msg->y;
				const double dist = sqrt(dx * dx + dy * dy);
				const double travel_time = dist / speed;

				struct agent_message new_agent;
				new_agent.x = dest_x;
				new_agent.y = dest_y;

				ScheduleNewEvent(dest_lp, now + travel_time, EV_MOVE_END, &new_agent,
				    sizeof(new_agent));
			}
			break;

		case EV_MOVE_END:
			{
				state->agents_in_region++;
				state->agents_paused++;

				const double pause_time = uniform_range(&state->rng, MIN_PAUSE, MAX_PAUSE);
				ScheduleNewEvent(me, now + pause_time, EV_PAUSE_END, msg, size);
			}
			break;

		default:
			fprintf(stderr, "Unknown event type\n");
			abort();
	}
}

bool CanEnd(_unused lp_id_t me, const void *snapshot)
{
	const struct region_state *state = snapshot;
	return state->agents_in_region == state->agents_paused;
}

struct simulation_configuration conf = {
    .lps = NUM_LPS,
    .n_threads = NUM_THREADS,
    .termination_time = 0,
    .gvt_period = 1000,
    .log_level = LOG_INFO,
    .stats_file = "mobility",
    .ckpt_interval = 0,
    .core_binding = true,
    .synchronization = TIME_WARP,
    .dispatcher = ProcessEvent,
    .committed = CanEnd,
};

int main(void)
{
	RootsimInit(&conf);
	return RootsimRun();
}
