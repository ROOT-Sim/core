/**
 * @file mm/dymelor/checkpoint.h
 *
 * @brief Checkpointing capabilities for DyMeLoR
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/dymelor/checkpoint.h>

#include <core/core.h>
#include <mm/dymelor/dymelor.h>
#include <mm/mm.h>

static size_t compute_log_size(const struct dymelor_state *ctx)
{
	size_t ret = sizeof(unsigned) + offsetof(struct dymelor_state_checkpoint, data);
	for(unsigned i = 0; i < NUM_AREAS; ++i) {
		const struct dymelor_area *area = ctx->areas[i];
		uint_least32_t num_chunks = MIN_NUM_CHUNKS;
		while(area != NULL) {
			ret += sizeof(offsetof(struct dymelor_area_checkpoint, data));
			ret += bitmap_required_size(num_chunks);
#ifdef ROOTSIM_INCREMENTAL
			ret += bitmap_required_size(num_chunks); // maybe not needed in full checkpoints
#endif
			ret += area->alloc_chunks << area->chk_size_exp;
			ret -= area->alloc_chunks * sizeof(uint_least32_t);

			num_chunks *= 2;
			area = area->next;
		}
	}
	return ret;
}

struct dymelor_state_checkpoint *dymelor_checkpoint_full_take(const struct dymelor_state *ctx)
{
	struct dymelor_state_checkpoint *ret = mm_alloc(compute_log_size(ctx));
	ret->used_mem = ctx->used_mem;
	struct dymelor_area_checkpoint *ckpt = (struct dymelor_area_checkpoint *)ret->data;
	for(unsigned i = 0; i < NUM_AREAS; ++i) {
		const struct dymelor_area *area = ctx->areas[i];
		uint_least32_t num_chunks = MIN_NUM_CHUNKS;
		uint_least32_t chunk_size = (((uint_least32_t)1U) << (MIN_CHUNK_EXP + i)) - sizeof(uint_least32_t);
		while(area != NULL) {
			ckpt->i = i;
			ckpt->chunk_cnt = area->alloc_chunks;
			size_t bitmap_size = bitmap_required_size(num_chunks);
			memcpy(ckpt->data, area->use_bitmap, bitmap_size);
			unsigned char *ptr = ckpt->data + bitmap_size;

#define copy_from_area(x)                                                                                              \
	({                                                                                                             \
		memcpy(ptr, area->area + ((x) * (chunk_size + sizeof(uint_least32_t))), chunk_size);                   \
		ptr += chunk_size;                                                                                     \
	})

			// Copy only the allocated chunks
			bitmap_foreach_set(area->use_bitmap, bitmap_size, copy_from_area);

#undef copy_from_area

			num_chunks *= 2;
			area = area->next;
			ckpt = (struct dymelor_area_checkpoint *)ptr;
		}
	}
	ckpt->i = UINT_MAX;

	return ret;
}

void dymelor_checkpoint_full_restore(struct dymelor_state *ctx, const struct dymelor_state_checkpoint *state_ckpt)
{
	ctx->used_mem = state_ckpt->used_mem;
	const struct dymelor_area_checkpoint *ckpt = (const struct dymelor_area_checkpoint *)state_ckpt->data;
	for(unsigned i = 0; i < sizeof(ctx->areas) / sizeof(*ctx->areas); ++i) {
		struct dymelor_area *area = ctx->areas[i];
		uint_least32_t num_chunks = MIN_NUM_CHUNKS;
		if(i == ckpt->i) {
			uint_least32_t chunk_size = (1U << (MIN_CHUNK_EXP + i)) - sizeof(uint_least32_t);
			do {
				area->alloc_chunks = ckpt->chunk_cnt;
				size_t bitmap_size = bitmap_required_size(num_chunks);
				memcpy(area->use_bitmap, ckpt->data, bitmap_size);
				const unsigned char *ptr = ckpt->data + bitmap_size;

#define copy_to_area(x)                                                                                                \
	({                                                                                                             \
		memcpy(area->area + ((x) * (chunk_size + sizeof(uint_least32_t))), ptr, chunk_size);                   \
		ptr += chunk_size;                                                                                     \
	})

				bitmap_foreach_set(area->use_bitmap, bitmap_size, copy_to_area);
#undef copy_to_area
				num_chunks *= 2;
				area = area->next;
				ckpt = (const struct dymelor_area_checkpoint *)ptr;
			} while(i == ckpt->i);
		}

		while(area != NULL) {
			area->alloc_chunks = 0;
			memset(area->use_bitmap, 0, bitmap_required_size(num_chunks));
			num_chunks *= 2;
			area = area->next;
		}
	}
}

void dymelor_checkpoint_trim_to(struct dymelor_state *ctx, const struct dymelor_state_checkpoint *state_ckpt)
{
	const struct dymelor_area_checkpoint *ckpt = (const struct dymelor_area_checkpoint *)state_ckpt->data;
	for(unsigned i = 0; i < sizeof(ctx->areas) / sizeof(*ctx->areas); ++i) {
		struct dymelor_area **next_area_p = &ctx->areas[i];
		if(i == ckpt->i) {
			uint_least32_t chunk_size = (1U << (MIN_CHUNK_EXP + i)) - sizeof(uint_least32_t);
			uint_least32_t num_chunks = MIN_NUM_CHUNKS;
			do {
				struct dymelor_area *area = *next_area_p;
				area->alloc_chunks = ckpt->chunk_cnt;
				size_t bitmap_size = bitmap_required_size(num_chunks);
				memcpy(area->use_bitmap, ckpt->data, bitmap_size);
				const unsigned char *ptr = ckpt->data + bitmap_size;
				uint_least32_t set = bitmap_count_set(area->use_bitmap, bitmap_size);
				ptr += set * chunk_size;
				num_chunks *= 2;
				next_area_p = &area->next;
				ckpt = (const struct dymelor_area_checkpoint *)ptr;
			} while(i == ckpt->i);
		}

		struct dymelor_area *area = *next_area_p;
		*next_area_p = NULL;
		while(area != NULL) {
			struct dymelor_area *next_area = area->next;
			mm_free(area);
			area = next_area;
		}
	}
}
