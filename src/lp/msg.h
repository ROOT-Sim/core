/**
 * @file lp/msg.h
 *
 * @brief Message management functions
 *
 * Message management functions
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <core/core.h>

#include <limits.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <datatypes/array.h>

#define BASE_PAYLOAD_SIZE 16

#define msg_is_before_serial(ma, mb) ((ma)->dest_t < (mb)->dest_t)

#define msg_is_before(ma, mb)                                                  \
	((ma)->dest_t < (mb)->dest_t ||                                        \
	    ((ma)->dest_t == (mb)->dest_t &&                                   \
		msg_is_before_extended((ma), (mb))))

#define msg_bare_size(msg) (offsetof(struct lp_msg, pl) + (msg)->pl_size)
#define msg_anti_size() (offsetof(struct lp_msg, m_type) + sizeof(uint32_t))

typedef struct table_lp_entry_t {
	lp_id_t lid;
	void *data;
} table_lp_entry_t;

typedef dyn_array(table_lp_entry_t) lp_entry_arr;

typedef struct table_thread_entry_t {
	rid_t tid;
	lp_entry_arr lp_arr;
} table_thread_entry_t;

typedef dyn_array(table_thread_entry_t) t_entry_arr;

// Size of additional data needed by pubsub messages published locally
struct t_pubsub_info {
	size_t m_cnt;
	struct lp_msg **m_arr;
	lp_entry_arr *lp_arr_p;
};

/// A model simulation message
struct lp_msg {
	/// The id of the recipient LP
	lp_id_t dest;
	/// The intended destination logical time of this message
	simtime_t dest_t;
	union {
		/// The flags to handle local anti messages
		_Atomic(uint32_t) flags;
		/// The message unique id, used for inter-node anti messages
		uint32_t raw_flags;
	};
	lp_id_t send;
#if LOG_LEVEL <= LOG_DEBUG
	simtime_t send_t;
#endif
	/// The message sequence number
	uint32_t m_seq;
	/// The message type, a user controlled field
	uint32_t m_type;
	/// The message payload size
	uint32_t pl_size;
	/// Position for heap data structure
	uint_fast32_t pos;

	struct t_pubsub_info pubsub_info;

	/// The initial part of the payload
	unsigned char pl[BASE_PAYLOAD_SIZE];
	/// The continuation of the payload
	unsigned char extra_pl[];
};

struct lp_msg_remote_match {
	uint32_t raw_flags;
	uint32_t m_seq;
};

enum msg_flag {
	MSG_FLAG_ANTI = 1,
	MSG_FLAG_PROCESSED = 2,
	MSG_FLAG_PUBSUB = 4
};

#define MSG_FLAGS_BITS 3

static inline bool msg_is_before_extended(const struct lp_msg *restrict a,
    const struct lp_msg *restrict b)
{
	if((a->raw_flags & MSG_FLAG_ANTI) != (b->raw_flags & MSG_FLAG_ANTI))
		return (a->raw_flags & MSG_FLAG_ANTI) >
		       (b->raw_flags & MSG_FLAG_ANTI);

	if(a->m_type != b->m_type)
		return a->m_type > b->m_type;

	if(a->pl_size != b->pl_size)
		return a->pl_size < b->pl_size;

	return memcmp(a->pl, b->pl, a->pl_size) > 0;
}
