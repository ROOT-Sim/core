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

#include <arch/timer.h>
#include <core/sync.h>
#include <datatypes/heap.h>
#include <datatypes/msg_skip_list.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>

#include <stdalign.h>
#include <stdatomic.h>

/// Determine an ordering between two elements in a queue
#define q_elem_is_before(ma, mb)  ((ma).t < (mb).t)

/// An element in the message queue
struct q_elem {
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
static __thread heap_declare(struct q_elem) mqp;
static __thread struct msg_skip_list msl;

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
	msg_skip_list_init(&msl);
	atomic_store_explicit(&queues[rid].list, NULL, memory_order_relaxed);
}

/**
 * @brief Finalizes the message queue for the current thread
 */
void msg_queue_fini(void)
{
	for(array_count_t i = 0; i < heap_count(mqp); ++i)
		msg_allocator_free(heap_items(mqp)[i].m);

	msg_skip_list_fini(&msl);
	heap_fini(mqp);

	struct lp_msg *m = atomic_load_explicit(&queues[rid].list, memory_order_relaxed);
	while(m != NULL) {
		msg_allocator_free(m);
		m = m->next;
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
	struct lp_msg *m = atomic_exchange_explicit(&queues[rid].list, NULL, memory_order_acquire);
scan:
	while(m != NULL) {
#ifdef USE_EAGER_REMOVE_ANTI
		if(m->raw_flags & MSG_FLAG_ANTI) {
			uint64_t id = m->raw_flags - MSG_FLAG_ANTI;
#ifdef USE_HEAP
			for(array_count_t k = 0; k < heap_count(mqp); ++k) {
				struct q_elem e = array_get_at(mqp, k);
				if(e.t == m->dest_t && e.m->raw_flags == id) {
					heap_extract_from(mqp, q_elem_is_before, k);
					msg_allocator_free(e.m);
					struct lp_msg *n = m->next;
					msg_allocator_free(m);
					m = n;
					goto scan;
				}
			}
#else
			struct lp_msg *a;
			if((a = msg_skip_list_try_anti(&msl, m))) {
				msg_allocator_free(a);
				struct lp_msg *n = m->next;
				msg_allocator_free(m);
				m = n;
				continue;
			}
#endif
		}
#endif
#ifdef USE_HEAP
		struct q_elem qe = {.t = m->dest_t, .m = m};
		heap_insert(mqp, q_elem_is_before, qe);
#else
		msg_skip_list_insert(&msl, m);
#endif
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
#ifdef USE_HEAP
	struct lp_msg *msg = likely(heap_count(mqp)) ? heap_extract(mqp, q_elem_is_before).m : NULL;
#else
	struct lp_msg *msg = msg_skip_list_extract(&msl);
#endif
	stats_take(STATS_MSG_EXTRACTION, timer_hr_value(t));
	return msg;
}

/**
 * @brief Inserts a message in the queue
 * @param msg the message to insert in the queue
 */
void msg_queue_insert(struct lp_msg *msg)
{
	_Atomic(struct lp_msg *) *list_p = &queues[lid_to_rid(msg->dest)].list;
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
	assert(lid_to_rid(msg->dest) == rid);
#ifdef USE_HEAP
	struct q_elem qe = {.t = msg->dest_t, .m = msg};
	heap_insert(mqp, q_elem_is_before, qe);
#else
	msg_skip_list_insert(&msl, msg);
#endif
}
