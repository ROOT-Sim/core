/**
 * @file datatypes/msg_queue.c
 *
 * @brief Message queue datatype
 *
 * This is the message queue for the parallel runtime.
 * The design is pretty simple. A queue for n threads is composed of a vector of
 * n simpler private thread queues plus n public buffers. If thread t1 wants to
 * send a message to thread t2 it puts a message in its buffer. Insertions are
 * then cheap, while extractions simply empty the buffer into the private queue.
 * This way the critically thread locked code is minimal.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <datatypes/msg_queue.h>

#include <datatypes/queue_policies/msg_queue_per_lp.h>
#include <datatypes/queue_policies/msg_queue_per_node.h>
#include <datatypes/queue_policies/msg_queue_per_thread.h>

#include <datatypes/queues/msg_queue_heap.h>
#include <datatypes/queues/msg_queue_nb_skip_list.h>
#include <datatypes/queues/msg_queue_nb_skip_list/gc/gc.h>

struct message_queue_datatype msg_queue_current;
static enum message_queue_policy msg_queue_policy_current;

#define msg_queue_define_function(suffix)                                                                              \
	void msg_queue_##suffix(void)                                                                                  \
	{                                                                                                              \
		switch(msg_queue_policy_current) {                                                                     \
			case MESSAGE_QUEUE_PER_LP:                                                                     \
				msg_queue_per_lp_##suffix();                                                           \
				break;                                                                                 \
			case MESSAGE_QUEUE_PER_THREAD:                                                                 \
				msg_queue_per_thread_##suffix();                                                       \
				break;                                                                                 \
			case MESSAGE_QUEUE_PER_NODE:                                                                   \
				msg_queue_per_node_##suffix();                                                         \
				break;                                                                                 \
			default:                                                                                       \
				__builtin_unreachable();                                                               \
		}                                                                                                      \
	}


void msg_queue_set(enum message_queue mq, enum message_queue_policy policy)
{
	switch(mq) {
		case MESSAGE_QUEUE_HEAP:
			msg_queue_current = heap_datatype;
			break;
		case MESSAGE_QUEUE_NB_SKIP_LIST:
			_init_allocators();
			msg_queue_current = nb_skip_list_datatype;
			
	}
	msg_queue_policy_current = policy;
}

msg_queue_define_function(global_init)
msg_queue_define_function(global_fini)
msg_queue_define_function(init)
msg_queue_define_function(fini)
msg_queue_define_function(lp_init)
msg_queue_define_function(lp_fini)

simtime_t msg_queue_time_peek(void)
{
	return SIMTIME_MAX;
}

struct lp_msg *msg_queue_extract(void)
{
	switch(msg_queue_policy_current) {
		case MESSAGE_QUEUE_PER_LP:
			return msg_queue_per_lp_extract();
		case MESSAGE_QUEUE_PER_THREAD:
			return msg_queue_per_thread_extract();
		case MESSAGE_QUEUE_PER_NODE:
			return msg_queue_per_node_extract();
		default:
			__builtin_unreachable();
	}
}

void msg_queue_insert(struct lp_msg *msg)
{
	switch(msg_queue_policy_current) {
		case MESSAGE_QUEUE_PER_LP:
			msg_queue_per_lp_insert(msg);
			break;
		case MESSAGE_QUEUE_PER_THREAD:
			msg_queue_per_thread_insert(msg);
			break;
		case MESSAGE_QUEUE_PER_NODE:
			msg_queue_per_node_insert(msg);
			break;
		default:
			__builtin_unreachable();
	}
}
