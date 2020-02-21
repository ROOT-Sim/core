#pragma once

#include <lp/message.h>

extern void msg_allocator_init(void);
extern void msg_allocator_fini(void);

extern lp_msg* msg_alloc(unsigned payload_size);
extern void msg_free(lp_msg *);
