#include <datatypes/queue_policies/msg_queue_per_thread.h>

#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>

#include <stdalign.h>
#include <stdatomic.h>

/// The multi-threaded message buffer, implemented as a non-blocking list
struct msg_buffer {
	union {
		/// The head of the messages list
		alignas(CACHE_LINE_SIZE) _Atomic(struct lp_msg *) list;
		alignas(CACHE_LINE_SIZE) queue_mem_block *qp;
	};
};

/// The buffers vector
static struct msg_buffer *queues;
/// The private thread queue
static __thread queue_mem_block thread_queue;
static __thread queue_mem_block thread_queue_ctx;

/**
 * @brief Initializes the message queue at the node level
 */
void msg_queue_per_thread_global_init(void)
{
	queues = mm_aligned_alloc(CACHE_LINE_SIZE, global_config.n_threads * sizeof(*queues));
}

/**
 * @brief Initializes the message queue for the current thread
 */
void msg_queue_per_thread_init(void)
{
	msg_queue_current.context_alloc(&thread_queue_ctx);
	msg_queue_current.queue_alloc(&thread_queue_ctx, &thread_queue);
	if(msg_queue_current.is_thread_safe)
		queues[rid].qp = &thread_queue;
	else
		atomic_store_explicit(&queues[rid].list, NULL, memory_order_relaxed);
}

/**
 * @brief Initialize the message queue for the current LP
 *
 * This is a no-op for this kind of queue.
 */
void msg_queue_per_thread_lp_init(void) {}

/**
 * @brief Finalize the message queue for the current LP
 *
 * This is a no-op for this kind of queue.
 */
void msg_queue_per_thread_lp_fini(void) {}

/**
 * @brief Finalizes the message queue for the current thread
 */
void msg_queue_per_thread_fini(void)
{
	msg_queue_current.queue_free(&thread_queue_ctx, &thread_queue);
	msg_queue_current.context_free(&thread_queue_ctx);

	if(msg_queue_current.is_thread_safe)
		return;

	struct lp_msg *m = atomic_load_explicit(&queues[rid].list, memory_order_relaxed);
	while(m != NULL) {
		struct lp_msg *n = m->next;
		msg_allocator_free(m);
		m = n;
	}
}

/**
 * @brief Finalizes the message queue at the node level
 */
void msg_queue_per_thread_global_fini(void)
{
	mm_aligned_free(queues);
}

/**
 * @brief Extracts the next message from the queue
 * @returns a pointer to the message to be processed or NULL if there isn't one
 *
 * The extracted message is a best effort lowest timestamp for the current
 * thread. Guaranteeing the lowest timestamp may increase the contention on the
 * queues.
 */
struct lp_msg *msg_queue_per_thread_extract(void)
{
	if (!msg_queue_current.is_thread_safe) {
		struct lp_msg *m = atomic_exchange_explicit(&queues[rid].list, NULL, memory_order_acquire);
		while(m != NULL) {
			msg_queue_current.message_insert(&thread_queue_ctx, &thread_queue, m);
			m = m->next;
		}
	}
	return msg_queue_current.message_extract(&thread_queue_ctx, &thread_queue);
}

/**
 * @brief Inserts a message in the queue
 * @param msg the message to insert in the queue
 */
void msg_queue_per_thread_insert(struct lp_msg *msg)
{
	if (msg_queue_current.is_thread_safe) {
		// FIXME: the context is still wrong
		msg_queue_current.message_insert(&thread_queue_ctx, queues[lid_to_rid(msg->dest)].qp, msg);
		return;
	}

	_Atomic(struct lp_msg *) *list_p = &queues[lid_to_rid(msg->dest)].list;
	msg->next = atomic_load_explicit(list_p, memory_order_relaxed);
	while(unlikely(!atomic_compare_exchange_weak_explicit(list_p, &msg->next, msg, memory_order_release,
	    memory_order_relaxed)))
		spin_pause();
}
