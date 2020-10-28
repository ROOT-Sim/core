#pragma once

#include <datatypes/array.h>

#ifdef NEUROME_BUDDY_ALLOCATOR
#include <mm/buddy/buddy.h>
#else
#include <mm/minm/minm.h>
#endif

extern void model_allocator_lp_init(void);
extern void model_allocator_lp_fini(void);
extern void model_allocator_checkpoint_take(array_count_t ref_i);
extern void model_allocator_checkpoint_next_force_full(void);
extern array_count_t model_allocator_checkpoint_restore(array_count_t ref_i);
extern array_count_t model_allocator_fossil_lp_collect(array_count_t tgt_ref_i);

extern void __write_mem(void *ptr, size_t siz);

extern void *__wrap_malloc(size_t req_size);
extern void *__wrap_calloc(size_t nmemb, size_t req_size);
extern void __wrap_free(void *ptr);
extern void *__wrap_realloc(void *ptr, size_t req_size);
