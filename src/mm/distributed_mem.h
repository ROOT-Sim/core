#pragma once

#include <stdalign.h>
#include <stdint.h>

#define DISTR_MEM_CHUNK_EXP 16U

struct distr_mem_chunk {
	alignas(1UL << DISTR_MEM_CHUNK_EXP) unsigned char mem[1UL << DISTR_MEM_CHUNK_EXP];
};

extern void distributed_mem_global_init(void);
extern void distributed_mem_global_fini(void);
extern struct distr_mem_chunk *distributed_mem_chunk_alloc(void *ref);
extern void distributed_mem_chunk_free(struct distr_mem_chunk *chk);
extern void *distributed_mem_chunk_ref(const struct distr_mem_chunk *chk);
extern void distributed_mem_chunk_update(const struct distr_mem_chunk *chk, void *ref);
