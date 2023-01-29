#include <datatypes/queue_policies/msg_queue_per_thread.h>

#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <lp/lp.h>

#include <stdalign.h>
#include <stdatomic.h>

/// The multi-threaded message buffer, implemented as a non-blocking list
struct msg_queue_per_lp {
	/// The head of the messages list
	alignas(CACHE_LINE_SIZE) queue_mem_block b;
	spinlock_t lock;
};

/// The buffers vector
struct msg_queue_per_lp *lp_queues;

static __thread queue_mem_block lp_queue_ctx;

/**
 * @brief Initializes the message queue at the node level
 */
void msg_queue_per_lp_global_init(void)
{
	lp_queues = mm_aligned_alloc(CACHE_LINE_SIZE, global_config.lps * sizeof(*lp_queues));
}

/**
 * @brief Initializes the message queue for the current thread
 */
void msg_queue_per_lp_init(void)
{
	msg_queue_current.context_alloc(&lp_queue_ctx);
}

/**
 * @brief Initialize the message queue for the current LP
 *
 * This is a no-op for this kind of queue.
 */
void msg_queue_per_lp_lp_init(void)
{
	lp_id_t lp_id = current_lp - lps;

	spin_init(&lp_queues[lp_id].lock);
	msg_queue_current.queue_alloc(&lp_queue_ctx, &lp_queues[lp_id].b);
}

/**
 * @brief Finalize the message queue for the current LP
 *
 * This is a no-op for this kind of queue.
 */
void msg_queue_per_lp_lp_fini(void)
{
	lp_id_t lp_id = current_lp - lps;
	msg_queue_current.queue_free(&lp_queue_ctx, &lp_queues[lp_id].b);
}

/**
 * @brief Finalizes the message queue for the current thread
 */
void msg_queue_per_lp_fini(void)
{
	msg_queue_current.context_free(&lp_queue_ctx);
}

/**
 * @brief Finalizes the message queue at the node level
 */
void msg_queue_per_lp_global_fini(void)
{
	mm_aligned_free(lp_queues);
}

/**
 * @brief Extracts the next message from the queue
 * @returns a pointer to the message to be processed or NULL if there isn't one
 *
 * The extracted message is a best effort lowest timestamp for the current
 * thread. Guaranteeing the lowest timestamp may increase the contention on the
 * queues.
 */
struct lp_msg *msg_queue_per_lp_extract(void)
{
	lp_id_t min_lp = lid_thread_first;
	if(msg_queue_current.is_thread_safe) {
		simtime_t min_t = msg_queue_current.queue_time_peek(&lp_queues[min_lp].b);
		for(uint64_t i = lid_thread_first + 1; i < lid_thread_end; ++i) {
			simtime_t this_t = msg_queue_current.queue_time_peek(&lp_queues[i].b);
			if (this_t < min_t) {
				min_t = this_t;
				min_lp = i;
			}
		}

		return msg_queue_current.message_extract(&lp_queue_ctx, &lp_queues[min_lp].b);
	} else {
		simtime_t min_t = SIMTIME_MAX;
		for(uint64_t i = lid_thread_first; i < lid_thread_end; ++i) {
			if (!spin_trylock(&lp_queues[i].lock))
			    continue;

			simtime_t this_t = msg_queue_current.queue_time_peek(&lp_queues[i].b);
			if (this_t < min_t) {
				min_t = this_t;
				min_lp = i;
			}

			spin_unlock(&lp_queues[i].lock);
		}

		spin_lock(&lp_queues[min_lp].lock);
		struct lp_msg *ret = msg_queue_current.message_extract(&lp_queue_ctx, &lp_queues[min_lp].b);
		spin_unlock(&lp_queues[min_lp].lock);
		return ret;
	}
}

/**
 * @brief Inserts a message in the queue
 * @param msg the message to insert in the queue
 */
void msg_queue_per_lp_insert(struct lp_msg *msg)
{
	lp_id_t lp_id = msg->dest;
	if(msg_queue_current.is_thread_safe) {
		msg_queue_current.message_insert(&lp_queue_ctx, &lp_queues[lp_id].b, msg);
		return;
	}

	spin_lock(&lp_queues[lp_id].lock);
	msg_queue_current.message_insert(&lp_queue_ctx, &lp_queues[lp_id].b, msg);
	spin_unlock(&lp_queues[lp_id].lock);
}
