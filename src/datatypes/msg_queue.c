/**
* @file datatypes/msg_queue.c
*
* @brief Message queue datatype
*
* Message queue datatype
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

#define inner_queue {			\
	binary_heap(lp_msg *) q;	\
	spinlock_t lck;			\
}

struct queue_t {
	struct inner_queue;
	unsigned char padding[CACHE_LINE_SIZE - sizeof(struct inner_queue)];
};

#define mqueue(from, to) queues[to * n_threads + from]

static struct queue_t *queues;

void msg_queue_global_init(void)
{
	queues = mm_alloc(
		n_threads *
		n_threads *
		sizeof(*queues)
	);
}

void msg_queue_init(void)
{
	unsigned i = n_threads;
	while(i--) {
		heap_init(mqueue(i, rid).q);
		spin_init(&(mqueue(i, rid).lck));
	}
}

void msg_queue_fini(void)
{
	log_log(LOG_TRACE, "[T %u] msg queue fini", rid);
	unsigned i = n_threads;
	while(i--) {
		struct queue_t *this_q = &mqueue(i, rid);
		array_count_t j = heap_count(this_q->q);
		while(j--) {
			lp_msg *msg = heap_items(this_q->q)[j];
			if(!(atomic_load_explicit(
				&msg->flags, memory_order_relaxed) & MSG_FLAG_PROCESSED))
				msg_allocator_free(msg);
		}
		heap_fini(this_q->q);
	}
}

void msg_queue_global_fini(void)
{
	mm_free(queues);
}

lp_msg *msg_queue_extract(void)
{
	unsigned i = n_threads;
	struct queue_t *bid_q = &mqueue(rid, rid);
	lp_msg *msg = heap_count(bid_q->q) ? heap_min(bid_q->q) : NULL;

	while(i--) {
		struct queue_t *this_q = &mqueue(i, rid);
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

simtime_t msg_queue_time_peek(void)
{
	const unsigned t_cnt = n_threads;
	simtime_t t_min = SIMTIME_MAX;
	bool done[t_cnt];
	memset(done, 0, sizeof(done));

	for(unsigned i = 0, r = t_cnt; r; i = (i + 1) % t_cnt){
		if(done[i])
			continue;

		struct queue_t *this_q = &mqueue(i, rid);
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

void msg_queue_insert(lp_msg *msg)
{
	unsigned dest_rid = lid_to_rid[msg->dest];
	struct queue_t *this_q = &mqueue(rid, dest_rid);
	spin_lock(&this_q->lck);
	heap_insert(this_q->q, msg_is_before, msg);
	spin_unlock(&this_q->lck);
}
