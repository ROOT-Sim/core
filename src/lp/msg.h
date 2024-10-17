/**
 * @file lp/msg.h
 *
 * @brief Message management functions
 *
 * Message management functions
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <arch/timer.h>
#include <core/core.h>

#include <limits.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

/// The minimum size of the payload to which message allocations are snapped to
#define MSG_PAYLOAD_BASE_SIZE 32

/**
 * @brief Compute the value of the happens-before relation between two messages
 * @param[in] a a pointer to the first message to compare
 * @param[in] b a pointer to the second message to compare
 * @return true if the message pointed by @p a happens-before the message pointed by @p b, false otherwise
 */
#define msg_is_before(a, b) ((a)->dest_t < (b)->dest_t || ((a)->dest_t == (b)->dest_t && msg_is_before_extended(a, b)))

/**
 * @brief Get the size of the message preamble
 * @return the size in bytes of the message preamble
 * The preamble is the initial part of the message which doesn't need to be transmitted over MPI
 */
#define msg_preamble_size() (offsetof(struct lp_msg, dest))

/**
 * @brief Get the address of the remote message data, i.e. the part of the message to be transmitted over MPI
 * @param[in] msg a pointer to the message
 * @return the address of the message data
 */
#define msg_remote_data(msg) (&(msg)->dest)

/**
 * @brief Get the size of the message data, i.e. the part of the message to be transmitted over MPI
 * @param[in] msg a pointer to the message
 * @return the size in bytes of the message data
 */
#define msg_remote_size(msg) (offsetof(struct lp_msg, pl) - msg_preamble_size() + (msg)->pl_size)

/**
 * @brief Get the size of the a anti-message data, i.e. the part of the anti-message to be transmitted over MPI
 * @return the size in bytes of the anti-message data
 */
#define msg_remote_anti_size() (offsetof(struct lp_msg, m_type) - msg_preamble_size())

/// A model simulation message
struct lp_msg {
	/// The next element in the message list (used in the message queue)
	struct lp_msg *next;
	/// The measured cost to process this message
	timer_uint cost;
	/// The id of the recipient LP
	lp_id_t dest;
	/// The intended destination logical time of this message
	simtime_t dest_t;
	/**
	 * From the lowest significant bit:
	 * 2 bits for the message state machine (MSG_FLAGS_ANTI and MSG_FLAGS_PROCESSED)
	 * 1 bit for the actual GVT phase
	 * MAX_NODES_BITS for the id of the generating node
	 * MAX_THREADS_BITS for the id of the generating thread
	 * the others (60 - MAX_NODES_BITS - MAX_THREADS_BITS) for the progressive counter
	 */
	union {
		/// The flags to handle local anti messages
		_Atomic uint64_t flags;
		/// The message unique id, used for inter-node anti messages
		uint64_t raw_flags;
	};
#ifndef NDEBUG
	/// The sender of the message
	lp_id_t send;
	/// The send time of the message
	simtime_t send_t;
#endif
	/// The message type, a user controlled field
	uint32_t m_type;
	/// The message payload size
	uint32_t pl_size;
	/// The initial part of the payload
	unsigned char pl[MSG_PAYLOAD_BASE_SIZE];
	/// The continuation of the payload
	unsigned char extra_pl[];
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
	if (a->m_type != b->m_type)
		return a->m_type > b->m_type;

	if (a->pl_size != b->pl_size)
		return a->pl_size < b->pl_size;

	return memcmp(a->pl, b->pl, a->pl_size) > 0;
}
