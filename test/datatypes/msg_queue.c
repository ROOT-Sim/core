/**
 * @file test/tests/datatypes/msg_queue_test.c
 *
 * @brief Test: parallel message queue
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <memory.h>
#include <stdatomic.h>
#include <stdlib.h>

#include <datatypes/msg_queue.h>

#define N_THREADS 0
#define THREAD_REPS 100000

static atomic_uint msg_missing = 0;
static atomic_uint barrier_init = 0, barrier_insert = 0;

static int msg_queue_test_global_init(_unused void *_)
{
	global_config.n_threads = N_THREADS; // read by initialization code
	msg_queue_global_init();
	return 0;
}

static int msg_queue_test_global_fini(_unused void *_)
{
	msg_queue_global_fini();
	return (int)(atomic_load_explicit(&msg_missing, memory_order_relaxed));
}

static int msg_queue_test(_unused void *_)
{
	tid = test_parallel_thread_id();
	msg_queue_init();

	atomic_fetch_add_explicit(&barrier_init, 1U, memory_order_relaxed);
	while(atomic_load_explicit(&barrier_init, memory_order_relaxed) != test_thread_cores_count())
		;

	for(unsigned i = THREAD_REPS; i--;) {
		struct lp_msg *msg = malloc(sizeof(*msg));
		memset(msg, 0, sizeof(*msg));
		msg->dest_t = (double)test_random_range(THREAD_REPS);
		msg_queue_insert(msg, test_random_range(N_THREADS));
		atomic_fetch_add_explicit(&msg_missing, 1U, memory_order_relaxed);
	}

	atomic_fetch_add_explicit(&barrier_insert, 1U, memory_order_relaxed);
	while(atomic_load_explicit(&barrier_insert, memory_order_relaxed) != test_thread_cores_count())
		;

	int ret = 0;
	simtime_t last_time = 0.0;
	struct lp_msg *msg;
	while((msg = msg_queue_extract())) {
		if(msg->dest_t < last_time)
			--ret;
		last_time = msg->dest_t;
		free(msg);
		atomic_fetch_sub_explicit(&msg_missing, 1U, memory_order_relaxed);
	}

	msg_queue_fini();
	return ret;
}

int main(void)
{
	test("Message queue test: initialization", msg_queue_test_global_init, NULL);
	test_parallel("Message queue test: extractions", msg_queue_test, NULL, N_THREADS);
	test("Message queue test: finalization", msg_queue_test_global_fini, NULL);
}
