#include <datatypes/queues/msg_queue_heap.h>

#include <datatypes/heap.h>
#include <mm/msg_allocator.h>

/// Determine an ordering between two elements in a queue
#define q_elem_is_before(ma, mb) ((ma).t < (mb).t || ((ma).t == (mb).t && msg_is_before_extended(ma.m, mb.m)))

/// An element in the heap message queue
struct q_elem {
	/// The timestamp of the message
	simtime_t t;
	/// The message enqueued
	struct lp_msg *m;
};

typedef heap_declare(struct q_elem) heap_queue_t;

static void *msg_queue_heap_context_alloc(void)
{
	return NULL;
}

static void msg_queue_heap_context_free(void *unused)
{
	(void)unused;
}

static void *msg_queue_heap_alloc(void *unused)
{
	(void)unused;
	heap_queue_t *ret = mm_alloc(sizeof(heap_queue_t));
	heap_init(*ret);
	return ret;
}

static void msg_queue_heap_free(void *q)
{
	heap_queue_t *h = q;
	while(!array_is_empty(*h))
		msg_allocator_free(array_pop(*h).m);
	heap_fini(*h);
	mm_free(h);
}

static simtime_t msg_queue_heap_time_peek(void *q)
{
	heap_queue_t *h = q;
	return likely(heap_count(*h)) ? heap_min(*h).t : SIMTIME_MAX;
}

static void msg_queue_heap_insert(void *unused, void *q, struct lp_msg *m)
{
	(void) unused;
	heap_queue_t *h = q;
	struct q_elem e = {.t = m->dest_t, .m = m};
	heap_insert(*h, q_elem_is_before, e);
}

static struct lp_msg *msg_queue_heap_extract(void *unused, void *q)
{
	(void) unused;
	heap_queue_t *h = q;
	return likely(heap_count(*h)) ? heap_extract(*h, q_elem_is_before).m : NULL;
}

struct message_queue_datatype heap_datatype = {
    .is_thread_safe = false,
    .message_extract = msg_queue_heap_extract,
    .queue_time_peek = msg_queue_heap_time_peek,
    .message_insert = msg_queue_heap_insert,
    .context_free = msg_queue_heap_context_free,
    .context_alloc = msg_queue_heap_context_alloc,
    .queue_alloc = msg_queue_heap_alloc,
    .queue_free = msg_queue_heap_free
};
