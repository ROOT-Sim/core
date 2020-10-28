#pragma once

#include <core/core.h>
#include <datatypes/array.h>

#ifdef NEUROME_INCREMENTAL
#error "The minimal model allocator doesn't support incremental checkpointing"
#endif

#define B_CACHE_LINES 1

struct _mm_chunk {
	unsigned char m[B_CACHE_LINES * CACHE_LINE_SIZE];
};

struct mm_state {
	dyn_array(struct _mm_chunk ) chunks;
	array_count_t last_ref_i;
};
