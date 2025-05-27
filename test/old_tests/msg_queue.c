/**
 * @file test/tests/datatypes/msg_queue_test.c
 *
 * @brief Test: parallel message queue
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <memory.h>
#include <stdatomic.h>
#include <stdlib.h>

#include "test.h"

#include "datatypes/msg_queue.h"

#define N_THREADS 2
#define THREAD_REPS 100000
#define N_LPS_PER_NODE 64

static atomic_uint msg_missing = 0;
static atomic_uint msg_to_free = N_THREADS;

static void msg_allocator_free(struct lp_msg *msg)
{
	atomic_fetch_sub_explicit(&msg_to_free, 1U, memory_order_relaxed);
	free(msg);
}

static test_ret_t msg_queue_test_init(__unused void *_)
{
	msg_queue_init();
	return 0;
}

static test_ret_t msg_queue_test_fini(__unused void *_)
{
	msg_queue_global_fini();
	return (test_ret_t)(atomic_load(&msg_missing) | atomic_load(&msg_to_free));
}

static test_ret_t msg_queue_populate(__unused void *_)
{
	msg_queue_init();

	unsigned i = THREAD_REPS;
	while(i--) {
		struct lp_msg *msg = malloc(sizeof(*msg));
		memset(msg, 0, sizeof(*msg));
		msg->dest_t = (double)RANDOM(THREAD_REPS);
		msg->dest = RANDOM(N_LPS_PER_NODE);
		msg_queue_insert(msg);
		atomic_fetch_add_explicit(&msg_missing, 1U, memory_order_relaxed);
	}

	return (msg_missing != THREAD_REPS * N_THREADS);
}

static test_ret_t msg_queue_empty(__unused void *_)
{
	int ret = 0;
	struct lp_msg *msg;
	simtime_t last_time = 0.0;

	while((msg = msg_queue_extract())) {
		if(msg->dest_t < last_time)
			--ret;
		last_time = msg->dest_t;
		free(msg);
		atomic_fetch_sub_explicit(&msg_missing, 1U, memory_order_relaxed);
	}

	// to test msg cleanup
	msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	msg_queue_insert(msg);

	//	test_thread_barrier();

	msg_queue_fini();
	return ret;
}


void foo() {}

int main(void)
{
	init(N_THREADS);

	n_lps_node = N_LPS_PER_NODE;

	msg_queue_global_init();

	parallel_test("Initializing message queue", msg_queue_test_init, NULL);
	parallel_test("Populating threads message queues", msg_queue_populate, NULL);
	parallel_test("Extracting messages from queues", msg_queue_empty, NULL);
	test("Finalizing message queue", msg_queue_test_fini, NULL);

	finish();
}
