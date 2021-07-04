#pragma once

#include <stddef.h>

extern void hardware_inc_global_init(void);
extern void hardware_inc_global_fini(void);
extern void hardware_inc_on_take(void);
extern void hardware_inc_on_restore(void);
extern void __write_mem_hard(void *ptr, size_t s);
