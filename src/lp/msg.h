#pragma once

#include <core/core.h>
#include <stddef.h>

#define BASE_PAYLOAD_SIZE 32

#define msg_is_before(msg_a, msg_b) ((msg_a)->dest_t < (msg_b)->dest_t)

#define msg_bare_size(msg) (offsetof(lp_msg, pl) + (msg)->pl_size)

#ifndef NEUROME_SERIAL
#define msg_is_anti(msg) 	((msg)->m_type & (UINT64_C(1) << UINT64_C(32)))
#define msg_set_anti(msg) 	((msg)->m_type |= (UINT64_C(1) << UINT64_C(32)))
#endif

struct _lp_msg {
	lp_id_t dest;
	simtime_t dest_t;
	uint_fast32_t m_type;
	uint_fast32_t pl_size;
	unsigned char pl[BASE_PAYLOAD_SIZE];
	unsigned char extra_pl[];
};

typedef struct _lp_msg lp_msg;

#ifndef NEUROME_SERIAL
extern __thread lp_msg *current_msg;
#endif




