/**
 * @file datatypes/msg_queue.c
 *
 * @brief Message queue datatype
 *
 * This is the message queue for the parallel runtime.
 * The design is pretty simple. A queue for n threads is composed of a vector of n simpler private thread queues plus n
 * public buffers. If thread t1 wants to send a message to thread t2 it puts a message in its buffer. Insertions are
 * then cheap, while extractions simply empty the buffer into the private queue. This way the critically thread locked
 * code is minimal.
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/msg_queue.h>

#include <core/sync.h>
#include <datatypes/heap.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>

#include <stdalign.h>
#include <stdatomic.h>

/// Determine an ordering between two elements in a queue
#define q_elem_is_before(ma, mb)  ((ma).t < (mb).t)

/// An element in the message queue
struct msg_queue_elem {
	/// The timestamp of the message
	simtime_t t;
	/// The message enqueued
	struct lp_msg *m;
};

/// The multi-threaded message buffer, implemented as a non-blocking list
struct msg_buffer {
	/// The head of the messages list
	alignas(CACHE_LINE_SIZE) _Atomic(struct lp_msg *) list;
};

/// The buffers vector
static struct msg_buffer *queues;
/// The private thread queue
static __thread heap_declare(struct msg_queue_elem) mqp;

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
	heap_init(mqp);
	atomic_store_explicit(&queues[tid].list, NULL, memory_order_relaxed);
}

/**
 * @brief Finalizes the message queue for the current thread
 */
void msg_queue_fini(void)
{
	for(array_count_t i = 0; i < heap_count(mqp); ++i)
		msg_allocator_free(heap_items(mqp)[i].m);

	heap_fini(mqp);

	struct lp_msg *m = atomic_load_explicit(&queues[tid].list, memory_order_relaxed);
	while(m != NULL) {
		struct lp_msg *n = m->next;
		msg_allocator_free(m);
		m = n;
	}
}

/**
 * @brief Finalizes the message queue at the node level
 */
void msg_queue_global_fini(void)
{
	mm_aligned_free(queues);
}

/**
 * @brief Move the messages from the thread specific list into the thread private queue
 */
static inline void msg_queue_insert_queued(void)
{
	struct lp_msg *m = atomic_exchange_explicit(&queues[tid].list, NULL, memory_order_acquire);
	while(m != NULL) {
		struct msg_queue_elem qe = {.t = m->dest_t, .m = m};
		heap_insert(mqp, q_elem_is_before, qe);
		m = m->next;
	}
}

/**
 * @brief Extracts the next message from the queue
 * @returns a pointer to the message to be processed or NULL if there isn't one
 *
 * The extracted message is a best effort lowest timestamp for the current thread. Guaranteeing the lowest timestamp may
 * increase the contention on the queues.
 */
struct lp_msg *msg_queue_extract(void)
{
	timer_uint t = timer_hr_new();
	msg_queue_insert_queued();
	struct lp_msg *msg = NULL;
	if(likely(heap_count(mqp))) {
		msg = heap_min(mqp).m;
		__builtin_prefetch(msg);
		heap_extract(mqp, q_elem_is_before);
		msg->cost = t;
	}
	stats_take(STATS_MSG_EXTRACTION, timer_hr_value(t));
	return msg;
}

/**
 * @brief Inserts a message in the queue
 * @param msg the message to insert in the queue
 */
void msg_queue_insert(struct lp_msg *msg, tid_t this_tid)
{
	_Atomic(struct lp_msg *) *list_p = &queues[this_tid].list;
	msg->next = atomic_load_explicit(list_p, memory_order_relaxed);
	while(unlikely(!atomic_compare_exchange_weak_explicit(list_p, &msg->next, msg, memory_order_release,
	    memory_order_relaxed)))
		spin_pause();
}

/**
 * @brief Inserts a message in the queue, knowing it is destined for the current thread
 * @param msg the message to insert in the queue
 */
void msg_queue_insert_self(struct lp_msg *msg)
{
	assert(lps[msg->dest].rid == tid);
	struct msg_queue_elem qe = {.t = msg->dest_t, .m = msg};
	heap_insert(mqp, q_elem_is_before, qe);
}

/**
 * @brief Scan the local thread queue, and reinserts messages that do not belong anymore to the thread queue
 */
void msg_queue_local_rebind(void)
{
	msg_queue_insert_queued();
	for(array_count_t k = heap_count(mqp); k--;) {
		struct lp_msg *m = heap_items(mqp)[k].m;
		unsigned rid = lps[m->dest].rid;
		if(rid != tid) {
			assert(!LP_RID_IS_NID(rid));
			heap_extract_from(mqp, k, q_elem_is_before);
			msg_queue_insert(m, rid);
		}
	}
}
