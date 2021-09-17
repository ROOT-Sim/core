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

#define q_elem_is_before(ma, mb) ((ma).t < (mb).t || 		\
	((ma).t == (mb).t && (ma).m->raw_flags > (mb).m->raw_flags))

#define SWAP(a, b) 							\
	do {								\
		__typeof(a) _tmp = (a);					\
		(a) = (b);						\
		(b) = _tmp;						\
	} while(0)

struct q_elem {
	simtime_t t;
	struct lp_msg *m;
};

/// A queue synchronized by a spinlock
struct msg_queue {
	/// Synchronizes access to the queue
	alignas(CACHE_LINE_SIZE) spinlock_t lck;
	/// The actual queue element of the matrix
	dyn_array(struct q_elem) q;
};

/// The queues vector
static struct msg_queue *queues;
/// The private thread queue
static __thread heap_declare(struct q_elem) q_priv;

/**
 * @brief Initializes the message queue at the node level
 */
void msg_queue_global_init(void)
{
	queues = mm_aligned_alloc(CACHE_LINE_SIZE, n_threads *
			sizeof(*queues));
}

/**
 * @brief Initializes the message queue for the current thread
 */
void msg_queue_init(void)
{
	struct msg_queue *mq = &queues[rid];
	memset(mq, 0 , sizeof(*mq));
	array_init(mq->q);

	heap_init(q_priv);
}

/**
 * @brief Finalizes the message queue for the current thread
 */
void msg_queue_fini(void)
{
	for (array_count_t i = 0; i < heap_count(q_priv); ++i)
		msg_allocator_free(heap_items(q_priv)[i].m);

	heap_fini(q_priv);

	struct msg_queue *mq = &queues[rid];
	for (array_count_t i = 0; i < array_count(mq->q); ++i)
		msg_allocator_free(array_get_at(mq->q, i).m);

	array_fini(mq->q);
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

	spin_lock(&mq->lck);
	array_count_t i = array_count(mq->q);
	array_push_n(q_priv, array_items(mq->q), i);
	array_count(mq->q) = 0;
	spin_unlock(&mq->lck);

	heap_commit_n(q_priv, q_elem_is_before, i);
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
	return likely(heap_count(q_priv)) ?
			heap_extract(q_priv, q_elem_is_before).m : NULL;
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
	return likely(heap_count(q_priv)) ? heap_min(q_priv).t : SIMTIME_MAX;
}

/**
 * @brief Inserts a message in the queue
 * @param msg the message to insert in the queue
 */
void msg_queue_insert(struct lp_msg *msg)
{

	struct msg_queue *mq = &queues[lid_to_rid(msg->dest)];
	struct q_elem qe = {.t = msg->dest_t, .m = msg};

	spin_lock(&mq->lck);
	array_push(mq->q, qe);
	spin_unlock(&mq->lck);
}
