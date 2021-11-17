/**
 * @file datatypes/msg_queue.c
 *
 * @brief Message queue datatype
 *
 * This is the message queue for the parallel runtime.
 * The design is pretty simple. A queue for n threads is composed by a vector of
 * n simpler private thread queues plus n public buffers. If thread t1 wants to
 * send a message to thread t2 it puts a message in its buffer. Insertions are
 * then cheap, while extractions simply empty the buffer into the private queue.
 * This way the critically thread locked code is minimal.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/msg_queue.h>

#include <core/core.h>
#include <core/sync.h>
#include <datatypes/heap.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>

#include <stdalign.h>
#include <stdatomic.h>

/// Determine an ordering between two elements in a queue
#define q_elem_is_before(ma, mb) ((ma).t < (mb).t || ((ma).t == (mb).t && (ma).m->raw_flags > (mb).m->raw_flags))

/// An element in the message queue
struct q_elem {
	simtime_t t;
	struct lp_msg *m;
};

/// A queue synchronized by a spinlock
struct msg_queue {
	/// Synchronizes access to the queue
	alignas(CACHE_LINE_SIZE) spinlock_t q_lock;
	/// The actual queue element of the matrix
	dyn_array(struct q_elem) b;
};

/// The queues vector
static struct msg_queue *queues;
/// The private thread queue
static __thread struct {
	heap_declare(struct q_elem) q;
	struct q_elem *alt_items;
	array_count_t alt_cap;
} mqp;

/**
 * @brief Initializes the message queue at the node level
 */
void msg_queue_global_init(void)
{
	queues = mm_aligned_alloc(CACHE_LINE_SIZE, global_config.n_threads * sizeof(*queues));
}

/**
 * @brief Initializes the message queue for the current thread
 */
void msg_queue_init(void)
{
	heap_init(mqp.q);

	mqp.alt_cap = INIT_SIZE_ARRAY;
	mqp.alt_items = mm_alloc(sizeof(*mqp.alt_items) * mqp.alt_cap);

	struct msg_queue *mq = &queues[rid];

	array_init(mq->b);
	spin_init(&mq->q_lock);
}

/**
 * @brief Finalizes the message queue for the current thread
 */
void msg_queue_fini(void)
{
	for(array_count_t i = 0; i < heap_count(mqp.q); ++i)
		msg_allocator_free(heap_items(mqp.q)[i].m);

	heap_fini(mqp.q);
	mm_free(mqp.alt_items);

	struct msg_queue *mq = &queues[rid];
	for(array_count_t i = 0; i < array_count(mq->b); ++i)
		msg_allocator_free(array_get_at(mq->b, i).m);

	array_fini(mq->b);
}

/**
 * @brief Finalizes the message queue at the node level
 */
void msg_queue_global_fini(void)
{
	mm_aligned_free(queues);
}

static inline void msg_queue_insert_queued(void)
{
	struct msg_queue *mq = &queues[rid];

	spin_lock(&mq->q_lock);

	struct q_elem *tmp_items = array_items(mq->b);
	array_count_t c = array_count(mq->b);
	array_count_t tmp_cap = array_capacity(mq->b);

	array_items(mq->b) = mqp.alt_items;
	array_count(mq->b) = 0;
	array_capacity(mq->b) = mqp.alt_cap;

	spin_unlock(&mq->q_lock);

	mqp.alt_items = tmp_items;
	mqp.alt_cap = tmp_cap;

	heap_insert_n(mqp.q, q_elem_is_before, tmp_items, c);
}

/**
 * @brief Extracts the next message from the queue
 * @returns a pointer to the message to be processed or NULL if there isn't one
 *
 * The extracted message is a best effort lowest timestamp for the current
 * thread. Guaranteeing the lowest timestamp may increase the contention on the
 * queues.
 */
struct lp_msg *msg_queue_extract(void)
{
	msg_queue_insert_queued();
	return likely(heap_count(mqp.q)) ? heap_extract(mqp.q, q_elem_is_before).m : NULL;
}

/**
 * @brief Peeks the timestamp of the next message from the queue
 * @returns the lowest timestamp of the next message to be processed or
 *          SIMTIME_MAX is there's no message to process
 *
 * This returns the lowest timestamp of the next message to be processed for the
 * current thread. This is calculated in a precise fashion since this value is
 * used in the gvt calculation.
 */
simtime_t msg_queue_time_peek(void)
{
	msg_queue_insert_queued();
	return likely(heap_count(mqp.q)) ? heap_min(mqp.q).t : SIMTIME_MAX;
}

/**
 * @brief Inserts a message in the queue
 * @param msg the message to insert in the queue
 */
void msg_queue_insert(struct lp_msg *msg)
{
	rid_t dest_rid = lid_to_rid(msg->dest);
	struct msg_queue *mq = &queues[dest_rid];
	struct q_elem qe = {.t = msg->dest_t, .m = msg};

	spin_lock(&mq->q_lock);
	array_push(mq->b, qe);
	spin_unlock(&mq->q_lock);
}
