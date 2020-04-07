#include <test.h>
#include <test_rng.h>
#include <datatypes/msg_queue.h>
#include <lp/lp.h>

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#define THREAD_CNT 6
#define THREAD_REPS 1000000

static __thread uint128_t rng_state;
static pthread_barrier_t t_barrier;
static unsigned lid_to_rid_m[] = {0, 1, 2, 3, 0, 1, 2, 3};
static lp_struct lps_m[THREAD_CNT];

__thread lp_msg *current_msg;
__thread lp_struct *current_lp;
unsigned n_threads = THREAD_CNT;
unsigned *lid_to_rid = lid_to_rid_m;
lp_struct *lps = lps_m;

static int msg_queue_test_init(void)
{
	int ret;
	if((ret = pthread_barrier_init(&t_barrier, NULL, test_config.threads_count))){
		return ret;
	}
	msg_queue_global_init();
	return 0;
}

static int msg_queue_test_fini(void)
{
	msg_queue_global_fini();
	return pthread_barrier_destroy(&t_barrier);
}

static int msg_queue_test(unsigned thread_id)
{
	int ret = 0;
	lcg_init(rng_state, (thread_id + 1) * 1713);
	core_init();
	msg_queue_init();

	pthread_barrier_wait(&t_barrier);

	unsigned i = THREAD_REPS;
	while(i--){
		lp_msg *msg = malloc(sizeof(*msg));
		memset(msg, 0, sizeof(*msg));
		msg->dest_t = lcg_random(rng_state) * THREAD_REPS;
		msg->dest =
			lcg_random(rng_state) *
			sizeof(lid_to_rid_m) /
			sizeof(*lid_to_rid_m);
		msg_queue_insert(msg);
	}

	pthread_barrier_wait(&t_barrier);

	lp_msg *msg;
	simtime_t last_time = 0.0;
	msg_queue_extract();

	while((msg = current_msg)){
		if(msg->dest_t < last_time)
			--ret;
		last_time = msg->dest_t;
		free(msg);
	}

	msg_queue_fini();
	return ret;
}


struct _test_config_t test_config = {
	.test_name = "msg queue",
	.threads_count = THREAD_CNT,
	.test_init_fnc = msg_queue_test_init,
	.test_fini_fnc = msg_queue_test_fini,
	.test_fnc = msg_queue_test
};
