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

#define BASE_PAYLOAD_SIZE 32

#define msg_is_before(a, b) ((a)->dest_t < (b)->dest_t || ((a)->dest_t == (b)->dest_t && msg_is_before_extended(a, b)))

#define msg_bare_size(msg) (offsetof(struct lp_msg, pl) + (msg)->pl_size)
#define msg_anti_size() (offsetof(struct lp_msg, m_seq) + sizeof(uint32_t))

/// A model simulation message
struct lp_msg {
	/// The id of the recipient LP
	lp_id_t dest;
	/// The intended destination logical time of this message
	simtime_t dest_t;
	union {
		/// The flags to handle local anti messages
		_Atomic uint32_t flags;
		/// The message unique id, used for inter-node anti messages
		uint32_t raw_flags;
	};
#ifndef NDEBUG
	/// The sender of the message
	lp_id_t send;
	/// The send time of the message
	simtime_t send_t;
#endif
	/// The message sequence number
	uint32_t m_seq;
	/// The message type, a user controlled field
	uint32_t m_type;
	/// The message payload size
	uint32_t pl_size;
	/// The initial part of the payload
	unsigned char pl[BASE_PAYLOAD_SIZE];
	/// The continuation of the payload
	unsigned char extra_pl[];
};

/// The data structure used to keep track of out-of-order distributed antimessages
struct lp_msg_remote_match {
	/// The original anti message raw_flags
	uint32_t raw_flags;
	/// The original anti message m_seq
	uint32_t m_seq;
};

enum msg_flag { MSG_FLAG_ANTI = 1, MSG_FLAG_PROCESSED = 2 };

/**
 * @brief Compute a deterministic order for messages with same timestamp
 * @param a the first message to compare
 * @param b the second message to compare
 * @return true if the @p a come before @p b
 *
 * There can be two distinct messages a and b so that
 * msg_is_before_extended(a, b) == false and msg_is_before_extended(b, a) == false.
 * In that case, the two messages will necessarily induce the same state change
 * in the receiving LP: it doesn't make any difference which one will be processed first.
 */
static inline bool msg_is_before_extended(const struct lp_msg *restrict a, const struct lp_msg *restrict b)
{
	if ((a->raw_flags & MSG_FLAG_ANTI) != (b->raw_flags & MSG_FLAG_ANTI))
		return (a->raw_flags & MSG_FLAG_ANTI) > (b->raw_flags & MSG_FLAG_ANTI);

	if (a->m_type != b->m_type)
		return a->m_type > b->m_type;

	if (a->pl_size != b->pl_size)
		return a->pl_size < b->pl_size;

	return memcmp(a->pl, b->pl, a->pl_size) > 0;
}
