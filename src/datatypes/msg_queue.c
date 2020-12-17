/**
* @file datatypes/msg_queue.c
*
* @brief Message queue datatype
*
* This is the message queue for the parallel runtime.
* The design is pretty simple. A queue for n thread is composed by n * n square
* matrix of simpler queues. If thread t1 wants to send a message to thread t2 it
* puts a message in the i-th queue where i = t2 * n + t1. Insertions are then
* cheap, while extractions lazily lock the queues (costing linear time in the
* number of worked threads, which seems to be acceptable in practice).
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <datatypes/msg_queue.h>

#include <core/core.h>
#include <core/sync.h>
#include <datatypes/heap.h>
#include <lp/lp.h>
#include <mm/msg_allocator.h>

static struct msg_queue {
	_Alignas(CACHE_LINE_SIZE) struct {
		spinlock_t lck; //<! Synchronizes access to the queue
		binary_heap(struct lp_msg *) q; //<! The actual queue
	}; //<! this is needed to elegantly cache align this stuff
} *queues;

/**
 * @brief Utility macro to fetch the correct inner queue
 */
#define mqueue(from, to) (&queues[to * n_threads + from])

/**
 * @brief Initializes the message queue at a node level
 */
void msg_queue_global_init(void)
{
	queues = mm_alloc(n_threads * n_threads * sizeof(*queues));
}

/**
 * @brief Initializes the message queue at a thread level
 */
void msg_queue_init(void)
{
	rid_t i = n_threads;
	while(i--) {
		heap_init(mqueue(i, rid)->q);
		spin_init(&(mqueue(i, rid)->lck));
	}
}

/**
 * @brief Finalizes the message queue at a node level
 */
void msg_queue_fini(void)
{
	log_log(LOG_TRACE, "[T %u] msg queue fini", rid);
	rid_t i = n_threads;
	while(i--) {
		struct msg_queue *this_q = mqueue(i, rid);
		array_count_t j = heap_count(this_q->q);
		while(j--) {
			struct lp_msg *msg = heap_items(this_q->q)[j];
			if(!(atomic_load_explicit(
				&msg->flags, memory_order_relaxed) & MSG_FLAG_PROCESSED))
				msg_allocator_free(msg);
		}
		heap_fini(this_q->q);
	}
}

/**
 * @brief Finalizes the message queue at a global level
 */
void msg_queue_global_fini(void)
{
	mm_free(queues);
}

/**
 * @brief Extracts the next message from the queue
 * @returns a pointer to the message to be processed
 *
 * The extracted message is a best effort lowest timestamp for the current
 * thread. Guaranteeing the lowest timestamp may increase the contention on the
 * queues.
 */
struct lp_msg *msg_queue_extract(void)
{
	rid_t i = n_threads;
	struct msg_queue *bid_q = mqueue(rid, rid);
	struct lp_msg *msg = heap_count(bid_q->q) ? heap_min(bid_q->q) : NULL;

	while(i--) {
		struct msg_queue *this_q = mqueue(i, rid);
		if(!spin_trylock(&this_q->lck))
			continue;

		if (heap_count(this_q->q) &&
		    (!msg || msg_is_before(heap_min(this_q->q), msg))) {
			msg = heap_min(this_q->q);
			bid_q = this_q;
		}
		spin_unlock(&this_q->lck);
	}

	spin_lock(&bid_q->lck);

	if(likely(heap_count(bid_q->q)))
		msg = heap_extract(bid_q->q, msg_is_before);
	else
		msg = NULL;

	spin_unlock(&bid_q->lck);
	return msg;
}

/**
 * @brief Peeks the timestamp of the next message from the queue
 * @returns the lowest timestamp of the next message to be processed
 *
 * This returns the lowest timestamp of the next message to be processed for the
 * current thread. This is calculated in a precise fashion since this value is
 * used in the gvt calculation.
 */
simtime_t msg_queue_time_peek(void)
{
	const rid_t t_cnt = n_threads;
	simtime_t t_min = SIMTIME_MAX;
	bool done[t_cnt];
	memset(done, 0, sizeof(done));

	for(rid_t i = 0, r = t_cnt; r; i = (i + 1) % t_cnt){
		if(done[i])
			continue;

		struct msg_queue *this_q = mqueue(i, rid);
		if(!spin_trylock(&this_q->lck))
			continue;

		done[i] = true;
		--r;
		if (heap_count(this_q->q) && t_min >
					     heap_min(this_q->q)->dest_t) {
			t_min = heap_min(this_q->q)->dest_t;
		}
		spin_unlock(&this_q->lck);
	}

	return t_min;
}

/**
 * @brief Inserts a message in the queue
 * @param msg the message to insert in the queue
 *
 */
void msg_queue_insert(struct lp_msg *msg)
{
	rid_t dest_rid = lid_to_rid[msg->dest];
	struct msg_queue *this_q = mqueue(rid, dest_rid);
	spin_lock(&this_q->lck);
	heap_insert(this_q->q, msg_is_before, msg);
	spin_unlock(&this_q->lck);
}
