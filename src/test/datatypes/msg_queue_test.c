#include <test/test.h>
#include <test/test_rng.h>
#include <datatypes/msg_queue.h>
#include <lp/lp.h>

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#define THREAD_REPS 1000000

static pthread_barrier_t t_barrier;
static __thread uint128_t rng_state;

static int msg_queue_test_init(void)
{
	int ret;
	if((ret = pthread_barrier_init(&t_barrier, NULL, test_config.threads_count))){
		return ret;
	}
	msg_queue_global_init(test_config.threads_count);
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

	for(unsigned i = 0; i < THREAD_REPS; ++i){
		lp_msg *msg = malloc(sizeof(*msg));
		memset(msg, 0, sizeof(*msg));
		msg->destination_time = lcg_random(rng_state) * THREAD_REPS;
		msg_queue_insert(msg);
	}

	pthread_barrier_wait(&t_barrier);

	simtime_t last_time = 0.0;
	for(unsigned i = 0; i < THREAD_REPS; ++i){
		lp_msg *msg = msg_queue_extract();
		if(msg->destination_time < last_time)
			--ret;
		last_time = msg->destination_time;
		free(msg);
	}
	return ret;
}


struct _test_config_t test_config = {
	.test_name = "msg queue",
	.threads_count = 4,
	.test_init_fnc = msg_queue_test_init,
	.test_fini_fnc = msg_queue_test_fini,
	.test_fnc = msg_queue_test
};
