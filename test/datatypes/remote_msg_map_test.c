#include <test.h>
#include <test_rng.h>

#include <datatypes/remote_msg_map.h>

#include <memory.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#define THREAD_CNT 4
#define MSG_COUNT 1000000

static atomic_uint insert_calls;

void msg_queue_insert(struct lp_msg *msg)
{
	(void)msg;
	if (!msg)
		abort();
	if(atomic_load_explicit(&msg->flags, memory_order_acquire) ==
		(MSG_FLAG_ANTI | MSG_FLAG_PROCESSED)){
		atomic_fetch_add_explicit(&insert_calls, 1U, memory_order_relaxed);
	}
}

static int remote_msg_map_test(void)
{
	struct lp_msg *msgs = malloc(sizeof(*msgs) * MSG_COUNT);
	memset(msgs, 0, sizeof(*msgs) * MSG_COUNT);

	for(uint64_t i = 0; i < MSG_COUNT; ++i){
		if(i % 3)
			continue;
		atomic_store_explicit(&msgs[i].flags, MSG_FLAG_PROCESSED,
			memory_order_release);
	}

	test_thread_barrier();

	for(uint64_t i = 0; i < MSG_COUNT; ++i){
		remote_msg_map_match(msg_id_get(&msgs[i], i & 1U), rid,
			(i & 1U) ? &msgs[i] : NULL);
	}

	for(uint64_t i = 0; i < MSG_COUNT; ++i){
		remote_msg_map_match(msg_id_get(&msgs[i], i & 1U) + 1, rid,
			(i & 1U) ? NULL : &msgs[i]);
	}

	test_thread_barrier();

	int ret = 0;
	for(uint64_t i = 0; i < MSG_COUNT; ++i){
		ret -= !(atomic_load_explicit(&msgs[i].flags,
			memory_order_acquire) & MSG_FLAG_ANTI);
	}
	ret -= insert_calls != ((MSG_COUNT - 1) / 3 + 1) * THREAD_CNT;

	free(msgs);
	return ret;
}

static int remote_msg_map_test_init(void)
{
	remote_msg_map_global_init();
	return 0;
}

static int remote_msg_map_test_fini(void)
{
	remote_msg_map_global_fini();
	return 0;
}

const struct test_config test_config = {
	.threads_count = THREAD_CNT,
	.test_init_fnc = remote_msg_map_test_init,
	.test_fini_fnc = remote_msg_map_test_fini,
	.test_fnc = remote_msg_map_test
};
