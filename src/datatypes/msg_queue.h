/**
 * @file datatypes/msg_queue.h
 *
 * @brief Message queue datatype
 *
 * Message queue datatype
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>
#include <core/sync.h>
#include <lp/msg.h>

typedef uintptr_t queue_mem_block[(CACHE_LINE_SIZE - sizeof(spinlock_t)) / sizeof(uintptr_t)];

struct message_queue_datatype {
	/// Specifies if the message queue is thread-safe or needs to be locked with concurrent accesses
	bool is_thread_safe;
	/// Extracts a message, takes in input input a context, a message queue
	struct lp_msg *(*message_extract)(queue_mem_block *, queue_mem_block *);
	/// Extracts a message, takes in input a context, a message queue and a message
	void (*message_insert)(queue_mem_block *, queue_mem_block *, struct lp_msg *);
	/// Allocate a context, i.e.: stuff that can be shared by possibly multiple queues (for example a memory allocator)
	void (*context_alloc)(queue_mem_block *);
	/// Free a context, takes in input a context
	void (*context_free)(queue_mem_block *);
	/// Allocate a message queue, takes in input a context
	void (*queue_alloc)(queue_mem_block *, queue_mem_block *);
	/// Free a queue, takes in input a message queue
	void (*queue_free)(queue_mem_block *, queue_mem_block *);
	/// Peek a queue for a best effort lowest time, takes in input a message queue
	simtime_t (*queue_time_peek)(queue_mem_block *);
};

extern struct message_queue_datatype msg_queue_current;

enum message_queue_policy {
	MESSAGE_QUEUE_PER_LP,
	MESSAGE_QUEUE_PER_THREAD,
	MESSAGE_QUEUE_PER_NODE
};

enum message_queue {
	MESSAGE_QUEUE_HEAP,
	MESSAGE_QUEUE_NB_SKIP_LIST,
	MESSAGE_QUEUE_NB_LINKED_LIST
};

extern void msg_queue_set(enum message_queue mq, enum message_queue_policy policy);

extern void msg_queue_global_init(void);
extern void msg_queue_global_fini(void);
extern void msg_queue_init(void);
extern void msg_queue_fini(void);
extern void msg_queue_lp_init(void);
extern void msg_queue_lp_fini(void);
extern struct lp_msg *msg_queue_extract(void);
extern simtime_t msg_queue_time_peek(void);
extern void msg_queue_insert(struct lp_msg *msg);
