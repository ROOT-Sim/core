#include <datatypes/queues/msg_queue_nb_skip_list.h>

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
	pq_init((pq_t *)q, global_config.n_threads * 2);
}

static void msg_queue_nb_skip_list_free(queue_mem_block *ctx, queue_mem_block *q)
{
	(void)ctx;
	pq_destroy((pq_t *)q);
}

static simtime_t msg_queue_nb_skip_list_time_peek(queue_mem_block *q)
{
	return peek_key((pq_t *)q);
}

static void msg_queue_nb_skip_list_insert(queue_mem_block *unused, queue_mem_block *q, struct lp_msg *m)
{
	(void)unused;
	insert((pq_t *)q, m->dest_t, m);
}

static struct lp_msg *msg_queue_nb_skip_list_extract(queue_mem_block *unused, queue_mem_block *q)
{
	(void)unused;
	struct lp_msg *res = deletemin((pq_t *)q);
	return res;
}

struct message_queue_datatype nb_skip_list_datatype = {
    .is_thread_safe = true,
    .message_extract = msg_queue_nb_skip_list_extract,
    .queue_time_peek = msg_queue_nb_skip_list_time_peek,
    .message_insert = msg_queue_nb_skip_list_insert,
    .context_free = msg_queue_nb_skip_list_context_free,
    .context_alloc = msg_queue_nb_skip_list_context_alloc,
    .queue_alloc = msg_queue_nb_skip_list_alloc,
    .queue_free = msg_queue_nb_skip_list_free
};
