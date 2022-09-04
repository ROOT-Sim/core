#include <datatypes/queue_policies/msg_queue_per_thread.h>

#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <mm/msg_allocator.h>

#include <stdalign.h>

/// The private thread queue
static void *node_queue;
static void *node_queue_ctx;
static alignas(CACHE_LINE_SIZE) spinlock_t node_queue_lock;
static alignas(CACHE_LINE_SIZE) _Atomic(struct lp_msg *) node_list;
/**
 * @brief Initializes the message queue at the node level
 */
void msg_queue_per_node_global_init(void)
{
	node_queue_ctx = msg_queue_current.context_alloc();
	node_queue = msg_queue_current.queue_alloc(node_queue_ctx);
	spin_init(&node_queue_lock);
	atomic_store_explicit(&node_list, NULL, memory_order_relaxed);
}

/**
 * @brief Initializes the message queue for the current thread
 */
void msg_queue_per_node_init(void) {}

/**
 * @brief Initialize the message queue for the current LP
 *
 * This is a no-op for this kind of queue.
 */
void msg_queue_per_node_lp_init(void) {}

/**
 * @brief Finalize the message queue for the current LP
 *
 * This is a no-op for this kind of queue.
 */
void msg_queue_per_node_lp_fini(void) {}

/**
 * @brief Finalizes the message queue for the current thread
 */
void msg_queue_per_node_fini(void) {}

/**
 * @brief Finalizes the message queue at the node level
 */
void msg_queue_per_node_global_fini(void)
{
	msg_queue_current.queue_free(node_queue);
	msg_queue_current.context_free(node_queue_ctx);

	struct lp_msg *m = atomic_load_explicit(&node_list, memory_order_relaxed);
	while(m != NULL) {
		msg_allocator_free(m);
		m = m->next;
	}
}

/**
 * @brief Extracts the next message from the queue
 * @returns a pointer to the message to be processed or NULL if there isn't one
 *
 * The extracted message is a best effort lowest timestamp for the current
 * thread. Guaranteeing the lowest timestamp may increase the contention on the
 * queues.
 */
struct lp_msg *msg_queue_per_node_extract(void)
{
	if(msg_queue_current.is_thread_safe)
		return msg_queue_current.message_extract(node_queue_ctx, node_queue);

	struct lp_msg *m = atomic_exchange_explicit(&node_list, NULL, memory_order_acquire);

	spin_lock(&node_queue_lock);
	while(m != NULL) {
		msg_queue_current.message_insert(node_queue_ctx, node_queue, m);
		m = m->next;
	}
	struct lp_msg *ret = msg_queue_current.message_extract(node_queue_ctx, node_queue);
	spin_unlock(&node_queue_lock);
	return ret;
}

/**
 * @brief Inserts a message in the queue
 * @param msg the message to insert in the queue
 */
void msg_queue_per_node_insert(struct lp_msg *msg)
{
	if(msg_queue_current.is_thread_safe) {
		msg_queue_current.message_insert(node_queue_ctx, node_queue, msg);
		return;
	}

	_Atomic(struct lp_msg *) *list_p = &node_list;
	msg->next = atomic_load_explicit(list_p, memory_order_relaxed);
	while(unlikely(!atomic_compare_exchange_weak_explicit(list_p, &msg->next, msg, memory_order_release,
	    memory_order_relaxed)))
		spin_pause();
}
