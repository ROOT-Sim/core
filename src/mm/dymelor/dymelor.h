#pragma once

#include <core/core.h>
#include <datatypes/array.h>
#include <datatypes/bitmap.h>

#include <stdalign.h>
#include <string.h>

#define MIN_CHUNK_EXP 7U	// Size (in bytes) of the smallest chunk provideable by DyMeLoR
#define MAX_CHUNK_EXP 22U	// Size (in bytes) of the biggest one. Notice that if this number
			       // is too large, performance (and memory usage) might be affected.
                               // If it is too small, large amount of memory requests by the
                               // application level software (i.e, larger than this number)
                               // will fail, as DyMeLoR will not be able to handle them!

#define MAX_CHUNK_SIZE ((1U << MAX_CHUNK_EXP) - sizeof(uint_least32_t))
#define NUM_AREAS (MAX_CHUNK_EXP - MIN_CHUNK_EXP + 1)
#define MIN_NUM_CHUNKS 512	// Minimum number of chunks per malloc_area


/// This structure let DyMeLoR handle one malloc area (for serving given-size memory requests)
struct dymelor_area {
	struct dymelor_area *next;
	block_bitmap *use_bitmap;
	block_bitmap *core_bitmap;
#ifdef ROOTSIM_INCREMENTAL
	block_bitmap *dirty_bitmap;
	uint_least32_t dirty_chunks;
#endif
	uint_least32_t core_chunks;
	uint_least32_t alloc_chunks;
	uint_least32_t last_chunk;
	unsigned chk_size_exp;
	uint_least32_t first_back_p;
	alignas(16) unsigned char area[];
};

struct dymelor_log {
	/// The reference index, used to identify this checkpoint
	array_count_t ref_i;
	/// A pointer to the actual checkpoint
	struct dymelor_ctx_checkpoint *c;
};

/// Definition of the memory map
struct mm_state {
	struct dymelor_area *areas[NUM_AREAS];
	uint_fast32_t used_mem;
	uint_fast32_t approx_used_mem;
	/// The array of checkpoints
	dyn_array(struct dymelor_log) logs;

	bool is_approximated;
};
