#include <datatypes/queues/msg_queue_nb_skip_list.h>
#include <datatypes/queues/msg_queue_nb_skip_list/prioq.h>
#include <mm/msg_allocator.h>

#include <core/core.h>

/// Determine an ordering between two elements in a queue
#define q_elem_is_before(ma, mb) ((ma).t < (mb).t || ((ma).t == (mb).t && msg_is_before_extended((ma).m, (mb).m)))

static void msg_queue_nb_skip_list_context_alloc(queue_mem_block *ctx)
{
	(void)ctx;
}

static void msg_queue_nb_skip_list_context_free(queue_mem_block *ctx)
{
	(void)ctx;
}

static void msg_queue_nb_skip_list_alloc(queue_mem_block *ctx, queue_mem_block *q)
{
	(void)ctx;
	pq_t *ptr = pq_init(global_config.n_threads*2);
	memcpy(q, ptr, sizeof(queue_mem_block));
	free(ptr);
}

static void msg_queue_nb_skip_list_free(queue_mem_block *ctx, queue_mem_block *q)
{
	(void)ctx;
	//pq_destroy((pq_t*)q);

}

static simtime_t msg_queue_nb_skip_list_time_peek(queue_mem_block *q)
{
	struct lp_msg *m = deletemin((pq_t*)q);
	simtime_t res = likely(m != NULL) ? m->dest_t : SIMTIME_MAX;
	if(m)	insert((pq_t*)q, m->dest_t, m);
	return res;
}

static void msg_queue_nb_skip_list_insert(queue_mem_block *unused, queue_mem_block *q, struct lp_msg *m)
{
	(void) unused;
	insert((pq_t*)q, m->dest_t, m);
}

static struct lp_msg *msg_queue_nb_skip_list_extract(queue_mem_block *unused, queue_mem_block *q)
{
	(void) unused;
	return deletemin((pq_t*)q);
}

struct message_queue_datatype nb_skip_list_datatype = {
    .is_thread_safe = false,
    .message_extract = msg_queue_nb_skip_list_extract,
    .queue_time_peek = msg_queue_nb_skip_list_time_peek,
    .message_insert = msg_queue_nb_skip_list_insert,
    .context_free = msg_queue_nb_skip_list_context_free,
    .context_alloc = msg_queue_nb_skip_list_context_alloc,
    .queue_alloc = msg_queue_nb_skip_list_alloc,
    .queue_free = msg_queue_nb_skip_list_free
};
