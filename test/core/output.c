/**
 * @file test/core/output.c
 *
 * @brief Test: committed output in presence of stragglers
 *
 * This test checks that the committed output is triggered correctly, even in
 * the presence of stragglers. It uses two LPs, where one waits for the other to
 * finish before sending a straggler event.
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <ROOT-Sim.h>
#include <test.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <assert.h>

#define NUM_LPS 2
#define NUM_THREADS 2

#define EVENT 1
#define STRAGGLER_EVENT 2

struct lp_state {
	unsigned long count;
};

#define OUTPUT_TYPE 0
#define OUTPUT_FROM_STRAGGLER 1

#define OUT_SLOTS 100
#define OUT_SZ 1024

struct output_data {
	lp_id_t id;
	unsigned long count;
};

atomic_bool lp1_turn = false;

char lp_outs[2][OUT_SLOTS][OUT_SZ];
size_t lp_outs_count[2] = {0, 0};

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, void *s);
bool CanEnd(lp_id_t me, const void *snapshot);
void PerformOutput(lp_id_t me, unsigned output_type, const void *output_content, unsigned output_size);

struct simulation_configuration conf = {
    .lps = NUM_LPS,
    .n_threads = NUM_THREADS,
    .termination_time = 25,
    .gvt_period = 10,
    .log_level = LOG_INFO,
    .stats_file = "output_test",
    .ckpt_interval = 0,
    .core_binding = true,
    .serial = false,
    .dispatcher = ProcessEvent,
    .committed = CanEnd,
    .perform_output = PerformOutput,
};

#define lp0_max_count 20
simtime_t lp0_times[lp0_max_count] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
#define lp1_max_count 10
simtime_t lp1_times[lp1_max_count] = {1.5, 3.5, 5.5, 7.5, 9.5, 11.5, 13.5, 15.5, 17.5, 19.5};
simtime_t lp1_send_delay = 0.2;

void Handler0(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, void *s)
{
	(void)now;
	(void)content;
	(void)size;

	struct lp_state *state = (struct lp_state *)s;

	if(state->count >= lp0_max_count || event_type == LP_FINI) {
		atomic_store(&lp1_turn, true);
		return;
	}

	while(atomic_load(&lp1_turn)) {
		// Wait for lp1 to schedule the straggler
	}

	if(event_type == EVENT) {
		ScheduleOutput(OUTPUT_TYPE, &(struct output_data){.id = me, .count = state->count},
		    sizeof(struct output_data));
		// printf("[Base event] ID%lu, ct%lu\n", me, state->count);
		ScheduleNewEvent(0, lp0_times[state->count], EVENT, NULL, 0);
		state->count++;

	} else if(event_type == STRAGGLER_EVENT) {
		// printf("[Straggler received] t%.1lf, ID%lu, ct%lu\n", now, me, state->count);
		ScheduleOutput(OUTPUT_FROM_STRAGGLER, &(struct output_data){.id = me, .count = state->count},
		    sizeof(struct output_data));
	}
}

void Handler1(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, void *s)
{
	(void)content;
	(void)size;

	struct lp_state *state = (struct lp_state *)s;

	if(state->count >= lp1_max_count || event_type == LP_FINI) {
		return;
	}

	while(!atomic_load(&lp1_turn)) {
		// Wait for lp0 to finish
	}

	if(event_type == EVENT) {
		ScheduleNewEvent(1, lp1_times[state->count], EVENT, NULL, 0);
		ScheduleNewEvent(0, now + lp1_send_delay, STRAGGLER_EVENT, NULL, 0);

		// printf("[Straggler generator] s%.1lf->d%.1lf, ID%lu, ct%lu\n", now, now + lp1_send_delay, me,
		// state->count);
		ScheduleOutput(OUTPUT_TYPE, &(struct output_data){.id = me, .count = state->count},
		    sizeof(struct output_data));
		state->count++;
	}

	atomic_store(&lp1_turn, false);
}

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, void *s)
{
	struct lp_state *state = (struct lp_state *)s;

	if(event_type != LP_FINI && now > conf.termination_time)
		return;

	if(event_type == LP_INIT) {
		state = rs_malloc(sizeof(*state));
		if(state == NULL)
			abort();
		SetState(state);
		state->count = 0;

		ScheduleNewEvent(me, 0, EVENT, NULL, 0);
		return;
	}

	if(me == 0) {
		Handler0(me, now, event_type, content, size, state);
	} else if(me == 1) {
		Handler1(me, now, event_type, content, size, state);
	} else {
		fprintf(stderr, "Unknown LP ID\n");
		abort();
	}
}

bool CanEnd(lp_id_t me, const void *snapshot)
{
	(void)me;
	(void)snapshot;
	return false;
}

void PerformOutput(lp_id_t me, unsigned output_type, const void *output_content, unsigned output_size)
{
	struct output_data *data = NULL;
	lp_id_t id;
	unsigned long count;
	switch(output_type) {
		case OUTPUT_TYPE:
			data = (struct output_data *)output_content;
			id = data->id;
			count = data->count;
			// printf("[Normal output] %lu,%lu,%lu\n", me, id, count);
			snprintf(lp_outs[me][lp_outs_count[me]], OUT_SZ, "N%lu,%lu,%lu", me, id, count);
			lp_outs_count[me]++;
			break;

		case OUTPUT_FROM_STRAGGLER:
			data = (struct output_data *)output_content;
			id = data->id;
			count = data->count;
			// printf("[Straggler output] %lu,%lu,%lu\n", me, id, count);
			snprintf(lp_outs[me][lp_outs_count[me]], OUT_SZ, "S%lu,%lu,%lu", me, id, count);
			lp_outs_count[me]++;
			break;

		default:
			fprintf(stderr, "Unknown output type %u, with size %u\n", output_type, output_size);
			break;
	}
}

bool check_output(const char *output, const char *expected, size_t expected_len, char *file, int line)
{
	if(strncmp(output, expected, expected_len)) {
		fprintf(stderr, "%s:%d: error: Output did not match expected. Actual: \"%s\", Expected:\"%s\".\n", file,
		    line, output, expected);
		test_fail();
	}
	return true;
}

#define chk_out(output, expected, expected_len) check_output(output, expected, expected_len, __FILE__, __LINE__)

int perform_exec()
{
	RootsimInit(&conf);

	for(size_t lp = 0; lp < 2; lp++) {
		for(size_t i = 0; i < OUT_SLOTS; i++) {
			memset(lp_outs[lp][i], 0, OUT_SZ);
		}
	}

	RootsimRun();

	// printf("OUTPUTS\n");
	// for(size_t lp = 0; lp < 2; lp++) {
	// 	for(size_t i = 0; i < OUT_SLOTS; i++) {
	// 		printf("%s\n", lp_outs[lp][i]);
	// 	}
	// }

	chk_out(lp_outs[0][0], "N0,0,0", 7);
	chk_out(lp_outs[0][1], "S0,0,1", 7);
	chk_out(lp_outs[0][2], "N0,0,1", 7);
	chk_out(lp_outs[0][3], "S0,0,2", 7);
	chk_out(lp_outs[0][4], "N0,0,2", 7);
	chk_out(lp_outs[0][5], "N0,0,3", 7);
	chk_out(lp_outs[0][6], "S0,0,4", 7);
	chk_out(lp_outs[0][7], "N0,0,4", 7);
	chk_out(lp_outs[0][8], "N0,0,5", 7);
	chk_out(lp_outs[0][9], "S0,0,6", 7);
	chk_out(lp_outs[0][10], "N0,0,6", 7);
	chk_out(lp_outs[0][11], "N0,0,7", 7);
	chk_out(lp_outs[0][12], "S0,0,8", 7);
	chk_out(lp_outs[0][13], "N0,0,8", 7);
	chk_out(lp_outs[0][14], "N0,0,9", 7);
	chk_out(lp_outs[0][15], "S0,0,10", 8);
	chk_out(lp_outs[0][16], "N0,0,10", 8);
	chk_out(lp_outs[0][17], "N0,0,11", 8);
	chk_out(lp_outs[0][18], "S0,0,12", 8);
	chk_out(lp_outs[0][19], "N0,0,12", 8);
	chk_out(lp_outs[0][20], "N0,0,13", 8);
	chk_out(lp_outs[0][21], "S0,0,14", 8);
	chk_out(lp_outs[0][22], "N0,0,14", 8);
	chk_out(lp_outs[0][23], "N0,0,15", 8);
	chk_out(lp_outs[0][24], "S0,0,16", 8);
	chk_out(lp_outs[0][25], "N0,0,16", 8);
	chk_out(lp_outs[0][26], "N0,0,17", 8);
	chk_out(lp_outs[0][27], "S0,0,18", 8);
	chk_out(lp_outs[0][28], "N0,0,18", 8);
	chk_out(lp_outs[0][29], "N0,0,19", 8);

	chk_out(lp_outs[1][0], "N1,1,0", 7);
	chk_out(lp_outs[1][1], "N1,1,1", 7);
	chk_out(lp_outs[1][2], "N1,1,2", 7);
	chk_out(lp_outs[1][3], "N1,1,3", 7);
	chk_out(lp_outs[1][4], "N1,1,4", 7);
	chk_out(lp_outs[1][5], "N1,1,5", 7);
	chk_out(lp_outs[1][6], "N1,1,6", 7);
	chk_out(lp_outs[1][7], "N1,1,7", 7);
	chk_out(lp_outs[1][8], "N1,1,8", 7);
	chk_out(lp_outs[1][9], "N1,1,9", 7);

	return 0;
}

int main(void)
{
	test("Testing committed output", perform_exec, NULL);
}
