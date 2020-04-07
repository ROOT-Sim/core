#pragma once

#include <lp/msg.h>

#include <memory.h>

extern void msg_allocator_init(void);
extern void msg_allocator_fini(void);

extern lp_msg* msg_allocator_alloc(unsigned payload_size);
extern void msg_allocator_free(lp_msg *);

inline lp_msg* msg_allocator_pack(unsigned receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size)
{
	lp_msg *msg = msg_allocator_alloc(payload_size + sizeof(unsigned));

	msg->dest = receiver;
	msg->dest_t = timestamp;
#ifndef NEUROME_SERIAL
	msg->sender = current_msg->dest;
#endif
	*((unsigned *) msg->pl) = event_type;
	memcpy(&msg->pl[sizeof(unsigned)], payload, payload_size);

	return msg;
}
