#pragma once

#include <mm/buddy/buddy.h>

#include <stddef.h>
#include <stdint.h>

extern void reverse_init(void);
extern void reverse_fini(void);
extern void *reverse_message_done(void);

extern void mark_memory_will_write(const void *ptr, size_t s);
extern void reverse_allocator_operation_register(struct buddy_state *buddy, uint_fast16_t info);
