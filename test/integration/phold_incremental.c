/**
 * @file test/integration/phold_incremental.c
 *
 * @brief A PHOLD benchmark with incremental checkpointing enabled
 *
 * This test runs the PHOLD workload with incremental checkpointing to validate
 * correctness and performance of the incremental checkpointing path under a
 * realistic Time Warp simulation workload.
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

#ifndef NUM_LPS
#define NUM_LPS 8192
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 0
#endif

#define EVENT 1

struct phold_state {
	__uint128_t seed;
};

struct phold_message {
	long int dummy_data;
};

static simtime_t p_remote = 0.25;
static simtime_t mean = 1.0;
static simtime_t lookahead = 0.0;
static int start_events = 1;

static double Random(struct phold_state *state)
{
	const __uint128_t multiplier = (((__uint128_t)0x0fc94e3bf4e9ab32ULL) << 64) + 0x866458cd56f5e605ULL;
	state->seed *= multiplier;
	const uint64_t ret = state->seed >> 64u;
	return (double)ret / (double)UINT64_MAX;
}

static double Expent(struct phold_state *state)
{
	return -mean * log(1 - Random(state));
}

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content, unsigned event_size,
    void *st)
{
	(void)event_size;
	(void)now;
	struct phold_state *state = st;

	switch(event_type) {
		case LP_INIT:
			state = rs_malloc(sizeof(*state));
			SetState(state);
			state->seed = me + 1;
			for(int i = 0; i < start_events; i++) {
				lp_id_t dest = (lp_id_t)(Random(state) * NUM_LPS);
				simtime_t ts = now + lookahead + Expent(state);
				ScheduleNewEvent(dest, ts, EVENT, NULL, 0);
			}
			break;

		case EVENT:
			{
				struct phold_message msg = {
				    .dummy_data = (long int)(event_content ? *(const long int *)event_content : 0)};
				lp_id_t dest;
				if(Random(state) < p_remote)
					dest = (lp_id_t)(Random(state) * NUM_LPS);
				else
					dest = me;
				simtime_t ts = now + lookahead + Expent(state);
				ScheduleNewEvent(dest, ts, EVENT, &msg, sizeof(msg));
				break;
			}

		case LP_FINI:
			rs_free(state);
			break;

		default:
			fprintf(stderr, "Unknown event type\n");
			abort();
	}
}

bool CanEnd(_unused lp_id_t me, _unused const void *snapshot)
{
	return false;
}

struct simulation_configuration conf = {
    .lps = NUM_LPS,
    .n_threads = NUM_THREADS,
    .termination_time = 1000,
    .gvt_period = 1000,
    .log_level = LOG_INFO,
    .stats_file = "phold_incremental",
    .ckpt_interval = 0,
    .incremental_ckpt = true,
    .full_ckpt_period = 10,
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
