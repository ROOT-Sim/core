#include <datatypes/msg_queue.h>

#include <arch/arch.h>
#include <datatypes/heap.h>
#include <core/sync.h>

#include <assert.h>
#include <stddef.h>

#define CORES_PER_QUEUE 2
#define cmp_msgs(a, b) ((a)->destination_time < (b)->destination_time)

#define _inner_queue 				\
	{					\
		spinlock_t lock;		\
		binary_heap(lp_msg *) queue;	\
	}

typedef struct {
	struct _inner_queue;
	const char padding[CACHE_LINE_SIZE - sizeof(struct _inner_queue)];
} msg_queue;

static_assert(
	sizeof(msg_queue) == CACHE_LINE_SIZE, "msg_queue not cache aligned");

static msg_queue *queues;
static unsigned queues_count;

void msg_queue_global_init(unsigned threads_cnt)
{
	queues_count = threads_cnt / CORES_PER_QUEUE +
			(threads_cnt % CORES_PER_QUEUE > 0);
	queues = malloc(sizeof(msg_queue) * queues_count);
	for(unsigned i = 0; i < queues_count; ++i){
		spin_init(&queues[i].lock);
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
		msg_queue* this_q = &queues[i];
		if(likely(spin_trylock(&this_q->lock))){
			if(likely(!heap_is_empty(this_q->queue))){
				lp_msg *msg = heap_extract(this_q->queue, cmp_msgs);
				spin_unlock(&this_q->lock);
				return msg;
			}
			spin_unlock(&this_q->lock);
		}
	}
}

void msg_queue_insert(lp_msg *msg)
{
	for(unsigned i = tid / CORES_PER_QUEUE; ; i = (i + 1) % queues_count){
		msg_queue* this_q = &queues[i];
		if(likely(spin_trylock(&this_q->lock))){
			heap_insert(this_q->queue, cmp_msgs, msg);
			spin_unlock(&this_q->lock);
			return;
		}
	}
}
