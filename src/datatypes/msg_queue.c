#include <datatypes/msg_queue.h>

#include <arch/arch.h>
#include <datatypes/heap.h>
#include <core/init.h>

#include <stddef.h>

#define CORES_PER_QUEUE 2
#define cmp_msgs(a, b) ((a)->destination_time < (b)->destination_time)

typedef struct {
	struct __msg_queue {
		uint64_t lock;
		binary_heap(lp_msg *) queue;
	};
	const char padding[CACHE_LINE_SIZE - sizeof(struct __msg_queue)];
} msg_queue;

_Static_assert(sizeof(msg_queue) == CACHE_LINE_SIZE, "msg_queue not cache aligned");

static msg_queue *queues;
static unsigned queues_count;

void msg_queue_global_init(void)
{
	queues_count = global_config.threads_cnt / CORES_PER_QUEUE +
			(global_config.threads_cnt % CORES_PER_QUEUE > 0);
	queues = malloc(sizeof(msg_queue) * queues_count);
	for(unsigned i = 0; i < queues_count; ++i){
		queues[i].lock = 0;
		heap_init(queues[i].queue);
	}
}

void msg_queue_global_fini(void)
{
	for(unsigned i = 0; i < queues_count; ++i){
		heap_fini(queues[i].queue);
	}
	free(queues);
}

lp_msg* msg_queue_extract(void)
{
	for(unsigned i = tid / CORES_PER_QUEUE; ; i = (i + 1) % queues_count){
		msg_queue* this_q = queues[i];
		if(likely(try_lock(this_q->lock))){
			if(likely(!heap_is_empty(this_q->queue))){
				lp_msg *msg = heap_extract(this_q->queue, cmp_msgs);
				unlock(this_q->lock);
				return msg;
			}
			unlock(this_q->lock);
		}
	}
}

void msg_queue_insert(lp_msg *msg)
{
	for(unsigned i = tid / CORES_PER_QUEUE; ; i = (i + 1) % queues_count){
		msg_queue* this_q = queues[i];
		if(likely(try_lock(this_q->lock))){
			heap_insert(this_q->queue, cmp_msgs, msg);
			unlock(this_q->lock);
			return;
		}
	}
}
