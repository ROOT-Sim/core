#pragma once

#include <mm/dymelor/dymelor.h>

#include <stdint.h>

struct dymelor_ctx_checkpoint {
	uint_least32_t used_mem;
	uint_least32_t approx_mem;
	bool approximated;
	unsigned char data[];
};

struct dymelor_area_checkpoint {
	unsigned i;
	uint_least32_t last_chunk;
	uint_least32_t chunk_cnt;
	uint_least32_t core_cnt;
	unsigned char data[];
};

extern struct dymelor_ctx_checkpoint *checkpoint_full_take(const struct mm_state *ctx, bool approximated);
extern void checkpoint_full_restore(struct mm_state *ctx, const struct dymelor_ctx_checkpoint *ckpt);
