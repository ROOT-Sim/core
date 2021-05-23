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

#include <stdatomic.h>
#include <stddef.h>
#include <limits.h>

#define BASE_PAYLOAD_SIZE 32

#define msg_is_before_serial(ma, mb) ((ma)->dest_t < (mb)->dest_t)

#define msg_is_before(ma, mb) (msg_is_before_serial(ma, mb) || 		\
	((ma)->dest_t == (mb)->dest_t && (ma)->raw_flags > (mb)->raw_flags))

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
		_Atomic(uint32_t) flags;
		/// The message unique id, used for inter-node anti messages
		uint32_t raw_flags;
	};
#if LOG_LEVEL <= LOG_DEBUG
	lp_id_t send;
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

enum msg_flag {
	MSG_FLAG_ANTI 		= 1,
	MSG_FLAG_PROCESSED	= 2
};



