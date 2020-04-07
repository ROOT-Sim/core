#pragma once

#include <datatypes/bitmap.h>

#include <stddef.h>
#include <stdint.h>

#define B_TOTAL_EXP 16U
#define B_BLOCK_EXP 6U

struct mm_state { // todo incremental checkpoints
	void *base_mem;
	uint64_t used_mem;
	block_bitmap dirty[bitmap_required_size(1 << (B_TOTAL_EXP - B_BLOCK_EXP))];
	uint8_t longest[(1 << (B_TOTAL_EXP - B_BLOCK_EXP + 1)) - 1];
};

struct _mm_checkpoint { // todo only log longest[] if changed, or incrementally
	uint64_t used_mem;
	uint8_t longest[(1 << (B_TOTAL_EXP - B_BLOCK_EXP + 1)) - 1];
	unsigned char base_mem[];
};

typedef struct _mm_checkpoint mm_checkpoint;

extern void model_memory_lp_init(void);
extern void model_memory_lp_fini(void);

extern void* model_alloc(size_t req_size);
extern void model_free(void *ptr);
extern void* model_realloc(void *ptr, size_t req_size);

extern void model_memory_mark_written(void *ptr, size_t write_size);
extern mm_checkpoint* model_checkpoint_take(void);
extern void model_checkpoint_restore(mm_checkpoint *);
extern void model_checkpoint_free(mm_checkpoint *);
