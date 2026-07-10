/**
 * @file test/core/output.c
 *
 * @brief Test: committed output correctness
 *
 * This file contains two test cases for the committed output mechanism:
 *
 * 1. **Time Warp (parallel) test**: Uses two LPs where LP 1 busy-waits until
 *    LP 0 has processed an event, then sends a straggler that causes LP 0 to
 *    roll back. The busy-waiting plus core_binding=true force a deterministic
 *    interleaving, yielding a fixed and verifiable committed output sequence.
 *    Both the exact output content (per-entry) and the total output count are
 *    checked; the latter catches any phantom outputs from rolled-back events.
 *
 * 2. **Serial test**: Verifies that ScheduleOutput() fires the output callback
 *    immediately and in causal order during a fully sequential simulation.
 *
 * @note Run with AddressSanitizer (-fsanitize=address) to detect memory leaks
 *       from the output content buffers.
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

/* =========================================================================
 * Shared event/output type constants and payload struct
 * ========================================================================= */

#define EVENT 1
#define STRAGGLER_EVENT 2

#define OUTPUT_TYPE 0
#define OUTPUT_FROM_STRAGGLER 1

#define OUT_SLOTS 100
#define OUT_SZ 64

struct output_data {
	lp_id_t id;
	unsigned long count;
};

/* =========================================================================
 * Time Warp (parallel) test
 * ========================================================================= */

#define TW_NUM_LPS 2
#define TW_NUM_THREADS 2

struct lp_state {
	unsigned long count;
};

#define lp0_max_count 20
static simtime_t lp0_times[lp0_max_count] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
#define lp1_max_count 10
static simtime_t lp1_times[lp1_max_count] = {1.5, 3.5, 5.5, 7.5, 9.5, 11.5, 13.5, 15.5, 17.5, 19.5};
static simtime_t lp1_send_delay = 0.2;

/* Atomic flag used to coordinate the deterministic interleaving between the
 * two LPs. lp1_turn==true means LP 0 is paused and LP 1 may proceed. */
static atomic_bool lp1_turn = false;

/* Output storage for the Time Warp test */
static char tw_lp_outs[TW_NUM_LPS][OUT_SLOTS][OUT_SZ];
static size_t tw_lp_outs_count[TW_NUM_LPS];
/* Total number of callback invocations, used to detect phantom rollback outputs */
static atomic_size_t tw_total_output_count;

static void TW_PerformOutput(lp_id_t me, unsigned output_type, const void *output_content, unsigned output_size)
{
	/* Verify the size is always exactly what was scheduled */
	if(output_size != sizeof(struct output_data)) {
		fprintf(stderr, "PerformOutput: unexpected output_size %u (expected %zu)\n", output_size,
		    sizeof(struct output_data));
		test_fail();
	}

	/* Bounds guard: unexpected extra outputs would silently corrupt memory otherwise */
	size_t idx = tw_lp_outs_count[me];
	if(idx >= OUT_SLOTS) {
		fprintf(stderr, "PerformOutput: too many outputs for LP %llu (>= %d)\n", (unsigned long long)me,
		    OUT_SLOTS);
		test_fail();
	}

	const struct output_data *data = output_content;
	lp_id_t id = data->id;
	unsigned long count = data->count;

	switch(output_type) {
		case OUTPUT_TYPE:
			snprintf(tw_lp_outs[me][idx], OUT_SZ, "N%llu,%llu,%lu", (unsigned long long)me,
			    (unsigned long long)id, count);
			break;
		case OUTPUT_FROM_STRAGGLER:
			snprintf(tw_lp_outs[me][idx], OUT_SZ, "S%llu,%llu,%lu", (unsigned long long)me,
			    (unsigned long long)id, count);
			break;
		default:
			fprintf(stderr, "PerformOutput: unknown output_type %u\n", output_type);
			test_fail();
	}

	tw_lp_outs_count[me]++;
	atomic_fetch_add_explicit(&tw_total_output_count, 1, memory_order_relaxed);
}

static void TW_Handler0(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, void *s)
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
		/* Spin until LP 1 has scheduled the straggler for this round */
	}

	if(event_type == EVENT) {
		ScheduleOutput(OUTPUT_TYPE, &(struct output_data){.id = me, .count = state->count},
		    sizeof(struct output_data));
		ScheduleNewEvent(0, lp0_times[state->count], EVENT, NULL, 0);
		state->count++;
	} else if(event_type == STRAGGLER_EVENT) {
		ScheduleOutput(OUTPUT_FROM_STRAGGLER, &(struct output_data){.id = me, .count = state->count},
		    sizeof(struct output_data));
	}
}

static void TW_Handler1(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, void *s)
{
	(void)content;
	(void)size;

	struct lp_state *state = s;

	if(state->count >= lp1_max_count || event_type == LP_FINI)
		return;

	while(!atomic_load(&lp1_turn)) {
		/* Spin until LP 0 has finished its current event */
	}

	if(event_type == EVENT) {
		ScheduleNewEvent(1, lp1_times[state->count], EVENT, NULL, 0);
		ScheduleNewEvent(0, now + lp1_send_delay, STRAGGLER_EVENT, NULL, 0);
		ScheduleOutput(OUTPUT_TYPE, &(struct output_data){.id = me, .count = state->count},
		    sizeof(struct output_data));
		state->count++;
	}

	atomic_store(&lp1_turn, false);
}

static void TW_ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size, void *s)
{
	struct lp_state *state = s;

	if(event_type != LP_FINI && now > 25.0)
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

	if(me == 0)
		TW_Handler0(me, now, event_type, content, size, state);
	else if(me == 1)
		TW_Handler1(me, now, event_type, content, size, state);
	else {
		fprintf(stderr, "TW_ProcessEvent: unknown LP ID %llu\n", (unsigned long long)me);
		abort();
	}
}

static bool TW_CanEnd(lp_id_t me, const void *snapshot)
{
	(void)me;
	(void)snapshot;
	return false;
}

static struct simulation_configuration tw_conf = {
    .lps = TW_NUM_LPS,
    .n_threads = TW_NUM_THREADS,
    .termination_time = 25,
    .gvt_period = 10,
    .log_level = LOG_WARN,
    .stats_file = "output_test_tw",
    .ckpt_interval = 0,
    .core_binding = true,
    .synchronization = TIME_WARP,
    .dispatcher = TW_ProcessEvent,
    .committed = TW_CanEnd,
    .output_callback = TW_PerformOutput,
};

static bool chk_out(const char *actual, const char *expected, const char *file, int line)
{
	if(strcmp(actual, expected) != 0) {
		fprintf(stderr, "%s:%d: output mismatch: got \"%s\", expected \"%s\"\n", file, line, actual, expected);
		test_fail();
	}
	return true;
}

#define CHK_OUT(actual, expected) chk_out(actual, expected, __FILE__, __LINE__)

static int perform_tw_exec(void *arg)
{
	(void)arg;

	/* Reset shared state between runs */
	atomic_store(&lp1_turn, false);
	memset(tw_lp_outs, 0, sizeof(tw_lp_outs));
	memset(tw_lp_outs_count, 0, sizeof(tw_lp_outs_count));
	atomic_store(&tw_total_output_count, 0);

	RootsimInit(&tw_conf);
	RootsimRun();

	/* --- Verify exact committed output sequence for LP 0 ---
	 *
	 * The busy-wait synchronization plus core_binding=true force a deterministic
	 * execution order. Each round: LP 0 processes an EVENT at time T (output N),
	 * then LP 1 sends a STRAGGLER_EVENT at time T+0.2 to LP 0. This causes LP 0
	 * to roll back and process the STRAGGLER_EVENT first (output S), then replay
	 * its EVENT (output N again). Some rounds don't trigger a straggler due to
	 * the interleaving; those appear as consecutive N outputs. */
	CHK_OUT(tw_lp_outs[0][0], "N0,0,0");
	CHK_OUT(tw_lp_outs[0][1], "S0,0,1");
	CHK_OUT(tw_lp_outs[0][2], "N0,0,1");
	CHK_OUT(tw_lp_outs[0][3], "S0,0,2");
	CHK_OUT(tw_lp_outs[0][4], "N0,0,2");
	CHK_OUT(tw_lp_outs[0][5], "N0,0,3");
	CHK_OUT(tw_lp_outs[0][6], "S0,0,4");
	CHK_OUT(tw_lp_outs[0][7], "N0,0,4");
	CHK_OUT(tw_lp_outs[0][8], "N0,0,5");
	CHK_OUT(tw_lp_outs[0][9], "S0,0,6");
	CHK_OUT(tw_lp_outs[0][10], "N0,0,6");
	CHK_OUT(tw_lp_outs[0][11], "N0,0,7");
	CHK_OUT(tw_lp_outs[0][12], "S0,0,8");
	CHK_OUT(tw_lp_outs[0][13], "N0,0,8");
	CHK_OUT(tw_lp_outs[0][14], "N0,0,9");
	CHK_OUT(tw_lp_outs[0][15], "S0,0,10");
	CHK_OUT(tw_lp_outs[0][16], "N0,0,10");
	CHK_OUT(tw_lp_outs[0][17], "N0,0,11");
	CHK_OUT(tw_lp_outs[0][18], "S0,0,12");
	CHK_OUT(tw_lp_outs[0][19], "N0,0,12");
	CHK_OUT(tw_lp_outs[0][20], "N0,0,13");
	CHK_OUT(tw_lp_outs[0][21], "S0,0,14");
	CHK_OUT(tw_lp_outs[0][22], "N0,0,14");
	CHK_OUT(tw_lp_outs[0][23], "N0,0,15");
	CHK_OUT(tw_lp_outs[0][24], "S0,0,16");
	CHK_OUT(tw_lp_outs[0][25], "N0,0,16");
	CHK_OUT(tw_lp_outs[0][26], "N0,0,17");
	CHK_OUT(tw_lp_outs[0][27], "S0,0,18");
	CHK_OUT(tw_lp_outs[0][28], "N0,0,18");
	CHK_OUT(tw_lp_outs[0][29], "N0,0,19");

	/* --- Verify exact committed output sequence for LP 1 --- */
	CHK_OUT(tw_lp_outs[1][0], "N1,1,0");
	CHK_OUT(tw_lp_outs[1][1], "N1,1,1");
	CHK_OUT(tw_lp_outs[1][2], "N1,1,2");
	CHK_OUT(tw_lp_outs[1][3], "N1,1,3");
	CHK_OUT(tw_lp_outs[1][4], "N1,1,4");
	CHK_OUT(tw_lp_outs[1][5], "N1,1,5");
	CHK_OUT(tw_lp_outs[1][6], "N1,1,6");
	CHK_OUT(tw_lp_outs[1][7], "N1,1,7");
	CHK_OUT(tw_lp_outs[1][8], "N1,1,8");
	CHK_OUT(tw_lp_outs[1][9], "N1,1,9");

	/* --- Verify total count: detects phantom outputs from rolled-back events ---
	 *
	 * LP 0: 20 N-outputs + 10 S-outputs = 30
	 * LP 1: 10 N-outputs
	 * Total expected: 40
	 *
	 * If committed_output_on_rollback() fails to suppress outputs, rolled-back
	 * EVENT handlers on LP 0 would fire extra N-outputs, inflating this count. */
	size_t total = atomic_load(&tw_total_output_count);
	if(total != 40) {
		fprintf(stderr, "Total output count mismatch: got %zu, expected 40\n", total);
		test_fail();
	}

	/* Also verify the per-LP counts individually */
	if(tw_lp_outs_count[0] != 30) {
		fprintf(stderr, "LP 0 output count mismatch: got %zu, expected 30\n", tw_lp_outs_count[0]);
		test_fail();
	}
	if(tw_lp_outs_count[1] != 10) {
		fprintf(stderr, "LP 1 output count mismatch: got %zu, expected 10\n", tw_lp_outs_count[1]);
		test_fail();
	}

	return 0;
}

/* =========================================================================
 * Serial test
 * ========================================================================= */

#define SERIAL_NUM_LPS 4
#define SERIAL_EVENTS_PER_LP 5

/* Output storage for the serial test */
static char serial_outs[SERIAL_NUM_LPS][SERIAL_EVENTS_PER_LP][OUT_SZ];
static size_t serial_outs_count[SERIAL_NUM_LPS];

static void Serial_PerformOutput(lp_id_t me, unsigned output_type, const void *output_content, unsigned output_size)
{
	if(output_type != OUTPUT_TYPE) {
		fprintf(stderr, "Serial_PerformOutput: unexpected output_type %u\n", output_type);
		test_fail();
	}
	if(output_size != sizeof(struct output_data)) {
		fprintf(stderr, "Serial_PerformOutput: unexpected output_size %u (expected %zu)\n", output_size,
		    sizeof(struct output_data));
		test_fail();
	}

	size_t idx = serial_outs_count[me];
	if(idx >= SERIAL_EVENTS_PER_LP) {
		fprintf(stderr, "Serial_PerformOutput: too many outputs for LP %llu\n", (unsigned long long)me);
		test_fail();
	}

	const struct output_data *data = output_content;
	snprintf(serial_outs[me][idx], OUT_SZ, "N%llu,%llu,%lu", (unsigned long long)me, (unsigned long long)data->id,
	    data->count);
	serial_outs_count[me]++;
}

static void Serial_ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *content, unsigned size,
    void *s)
{
	(void)now;
	(void)content;
	(void)size;

	struct lp_state *state = s;

	if(event_type == LP_INIT) {
		state = rs_malloc(sizeof(*state));
		if(state == NULL)
			abort();
		SetState(state);
		state->count = 0;
		ScheduleNewEvent(me, 1.0, EVENT, NULL, 0);
		return;
	}

	if(event_type == LP_FINI)
		return;

	if(event_type == EVENT && state->count < SERIAL_EVENTS_PER_LP) {
		ScheduleOutput(OUTPUT_TYPE, &(struct output_data){.id = me, .count = state->count},
		    sizeof(struct output_data));
		state->count++;
		if(state->count < SERIAL_EVENTS_PER_LP)
			ScheduleNewEvent(me, now + 1.0, EVENT, NULL, 0);
	}
}

static bool Serial_CanEnd(lp_id_t me, const void *snapshot)
{
	(void)snapshot;
	const struct lp_state *state = snapshot;
	(void)state;
	(void)me;
	return false;
}

static struct simulation_configuration serial_conf = {
    .lps = SERIAL_NUM_LPS,
    .n_threads = 1,
    .termination_time = 100,
    .log_level = LOG_WARN,
    .stats_file = "output_test_serial",
    .synchronization = SERIAL,
    .dispatcher = Serial_ProcessEvent,
    .committed = Serial_CanEnd,
    .output_callback = Serial_PerformOutput,
};

static int perform_serial_exec(void *arg)
{
	(void)arg;

	memset(serial_outs, 0, sizeof(serial_outs));
	memset(serial_outs_count, 0, sizeof(serial_outs_count));

	RootsimInit(&serial_conf);
	RootsimRun();

	/* Each LP should have produced exactly SERIAL_EVENTS_PER_LP outputs,
	 * one per EVENT, in causal order (count 0,1,2,...). */
	for(lp_id_t lp = 0; lp < SERIAL_NUM_LPS; lp++) {
		if(serial_outs_count[lp] != SERIAL_EVENTS_PER_LP) {
			fprintf(stderr, "Serial: LP %llu produced %zu outputs, expected %d\n", (unsigned long long)lp,
			    serial_outs_count[lp], SERIAL_EVENTS_PER_LP);
			test_fail();
		}
		for(unsigned i = 0; i < SERIAL_EVENTS_PER_LP; i++) {
			char expected[OUT_SZ];
			snprintf(expected, OUT_SZ, "N%llu,%llu,%u", (unsigned long long)lp, (unsigned long long)lp, i);
			if(strcmp(serial_outs[lp][i], expected) != 0) {
				fprintf(stderr, "Serial: LP %llu output[%u] = \"%s\", expected \"%s\"\n",
				    (unsigned long long)lp, i, serial_outs[lp][i], expected);
				test_fail();
			}
		}
	}

	return 0;
}

/* =========================================================================
 * Entry point
 * ========================================================================= */

int main(void)
{
	test("Testing committed output — serial mode", perform_serial_exec, NULL);
	test("Testing committed output — Time Warp with stragglers", perform_tw_exec, NULL);
}
