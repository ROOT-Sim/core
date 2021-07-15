/**
 * @file test/datatypes/msg_queue_test.c
 *
 * @brief Test: parallel message queue
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>
#include <test_rng.h>

#include <datatypes/msg_queue.h>
#include <lp/lp.h>

#include <memory.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#define THREAD_CNT 6
#define THREAD_REPS 100000

uint64_t n_lps_node = 64;
static __thread test_rng_state rng_state;
static struct lp_ctx lps_m[THREAD_CNT];
static atomic_uint msg_missing = THREAD_REPS * THREAD_CNT;
static atomic_uint msg_to_free = THREAD_CNT;
uint64_t lid_node_first;

struct lp_ctx *lps = lps_m;

void msg_allocator_free(struct lp_msg *msg)
{
	atomic_fetch_sub_explicit(&msg_to_free, 1U, memory_order_relaxed);
	free(msg);
}

static int msg_queue_test_init(void)
{
	n_threads = THREAD_CNT;
	msg_queue_global_init();
	return 0;
}

static int msg_queue_test_fini(void)
{
	msg_queue_global_fini();
	return -(atomic_load(&msg_missing) | atomic_load(&msg_to_free));
}

static int msg_queue_test(void)
{
	int ret = 0;
	lcg_init(rng_state, (rid + 1) * 1713);
	msg_queue_init();

	test_thread_barrier();

	unsigned i = THREAD_REPS;
	while(i--){
		struct lp_msg *msg = malloc(sizeof(*msg));
		memset(msg, 0, sizeof(*msg));
		msg->dest_t = lcg_random(rng_state) * THREAD_REPS;
		msg->dest = lcg_random(rng_state) * n_lps_node;
		msg_queue_insert(msg);
	}

	test_thread_barrier();

	struct lp_msg *msg;
	simtime_t last_time = 0.0;

	while((msg = msg_queue_extract())){
		if(msg->dest_t < last_time)
			--ret;
		last_time = msg->dest_t;
		free(msg);
		atomic_fetch_sub_explicit(&msg_missing, 1U, memory_order_relaxed);
	}

	test_thread_barrier();

	// to test msg cleanup
	msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(*msg));
	msg_queue_insert(msg);

	test_thread_barrier();

	msg_queue_fini();
	return ret;
}

const struct test_config test_config = {
	.threads_count = THREAD_CNT,
	.test_init_fnc = msg_queue_test_init,
	.test_fini_fnc = msg_queue_test_fini,
	.test_fnc = msg_queue_test
};
