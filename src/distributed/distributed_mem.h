#pragma once

#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>

#define DISTR_MEM_CHUNK_EXP 16

struct distr_mem_chunk {
	alignas(1 << DISTR_MEM_CHUNK_EXP) unsigned char mem[1 << DISTR_MEM_CHUNK_EXP];
};

extern void distributed_mem_global_init(void);
extern void distributed_mem_global_fini(void);
extern struct distr_mem_chunk *distributed_mem_chunk_alloc(void *ref);
extern void distributed_mem_chunk_free(struct distr_mem_chunk *chk);
extern void *distributed_mem_chunk_ref(const struct distr_mem_chunk *chk);
extern bool distributed_mem_is_contained(const void *ptr);
