#include <mm/dymelor/checkpoint.h>

#include <arch/timer.h>
#include <log/stats.h>
#include <lp/lp.h>

#include <stdio.h>

static size_t compute_log_size(const struct mm_state *ctx, bool approximated)
{
	size_t ret = offsetof(struct dymelor_ctx_checkpoint, data) + sizeof(unsigned);
	for(unsigned i = 0; i < NUM_AREAS; ++i) {
		const struct dymelor_area *area = ctx->areas[i];
		uint_least32_t num_chunks = MIN_NUM_CHUNKS;
		uint_least32_t chunk_size = (1U << (MIN_CHUNK_EXP + i)) - sizeof(uint_least32_t);
		while(area != NULL) {
			ret += offsetof(struct dymelor_area_checkpoint, data);
			ret += (2 - approximated) * bitmap_required_size(num_chunks);
#ifdef ROOTSIM_INCREMENTAL
			ret += bitmap_required_size(num_chunks); // maybe not needed in full checkpoints
#endif
			if (approximated)
				ret += chunk_size * area->core_chunks;
			else
				ret += chunk_size * area->alloc_chunks;
			num_chunks *= 2;
			area = area->next;
		}
	}
	return ret;
}

struct dymelor_ctx_checkpoint *checkpoint_full_take(const struct mm_state *ctx, bool approximated)
{
	timer_uint t = timer_hr_new();
	struct dymelor_ctx_checkpoint *state_ckpt = mm_alloc(compute_log_size(ctx, approximated));
	state_ckpt->used_mem = ctx->used_mem;
	state_ckpt->approx_mem = ctx->approx_used_mem;
	state_ckpt->approximated = approximated;
	struct dymelor_area_checkpoint *ckpt = (struct dymelor_area_checkpoint *) state_ckpt->data;
	for(unsigned i = 0; i < NUM_AREAS; ++i) {
		const struct dymelor_area *area = ctx->areas[i];
		uint_least32_t num_chunks = MIN_NUM_CHUNKS;
		uint_least32_t chunk_size = (1U << (MIN_CHUNK_EXP + i)) - sizeof(uint_least32_t);
		while(area != NULL) {

			ckpt->i = i;
			ckpt->chunk_cnt = area->alloc_chunks;
			ckpt->core_cnt = area->core_chunks;
			ckpt->last_chunk = area->last_chunk;

			size_t bitmap_size = bitmap_required_size(num_chunks);

#define copy_from_area(x)                                                                                              \
	({                                                                                                             \
		memcpy(ptr, area->area + ((x) * (chunk_size + sizeof(uint_least32_t))), chunk_size);                   \
		ptr += chunk_size;                                                                                     \
	})
			unsigned char *restrict ptr;
			// Copy only the allocated chunks
			if(approximated) {
				memcpy(ckpt->data, area->core_bitmap, bitmap_size);
				ptr = ckpt->data + bitmap_size;
				bitmap_foreach_set(area->core_bitmap, bitmap_size, copy_from_area);
			} else {
				memcpy(ckpt->data, area->use_bitmap, 2 * bitmap_size);
				ptr = ckpt->data + 2 * bitmap_size;
				bitmap_foreach_set(area->use_bitmap, bitmap_size, copy_from_area);
			}
#undef copy_from_area

			num_chunks *= 2;
			area = area->next;
			ckpt = (struct dymelor_area_checkpoint *) ptr;
		}
	}

	ckpt->i = UINT_MAX;
	stats_take(STATS_CKPT_STATE_SIZE, approximated ? ctx->approx_used_mem : ctx->used_mem);
	stats_take(STATS_CKPT_TIME, timer_hr_value(t));
	stats_take(STATS_CKPT, 1);
	return state_ckpt;
}

void checkpoint_full_restore(struct mm_state *ctx, const struct dymelor_ctx_checkpoint *state_ckpt)
{
	timer_uint t = timer_hr_new();
	bool approximated = state_ckpt->approximated;
	const struct dymelor_area_checkpoint *ckpt = (const struct dymelor_area_checkpoint *)state_ckpt->data;
	for(unsigned i = 0; i < sizeof(ctx->areas) / sizeof(*ctx->areas); ++i) {
		struct dymelor_area *area = ctx->areas[i];
		uint_least32_t num_chunks = MIN_NUM_CHUNKS;
		if(i == ckpt->i) {
			uint_least32_t chunk_size = (1U << (MIN_CHUNK_EXP + i)) - sizeof(uint_least32_t);
			do {
				area->core_chunks = ckpt->core_cnt;
				size_t bitmap_size = bitmap_required_size(num_chunks);
				const unsigned char *restrict ptr;

#define copy_to_area(x)                                                                                                \
	({                                                                                                             \
		memcpy(area->area + ((x) * (chunk_size + sizeof(uint_least32_t))), ptr, chunk_size);                   \
		ptr += chunk_size;                                                                                     \
	})
				// Copy only the allocated chunks
				if(approximated) {
					memcpy(area->use_bitmap, ckpt->data, bitmap_size);
					memcpy(area->core_bitmap, ckpt->data, bitmap_size);
					area->last_chunk = 0;
					while(bitmap_check(area->use_bitmap, area->last_chunk))
						area->last_chunk++;
					ptr = ckpt->data + bitmap_size;
					area->alloc_chunks = area->core_chunks;
					bitmap_foreach_set(area->core_bitmap, bitmap_size, copy_to_area);
				} else {
					memcpy(area->use_bitmap, ckpt->data, 2 * bitmap_size);
					ptr = ckpt->data + 2 * bitmap_size;
					area->last_chunk = ckpt->last_chunk;
					area->alloc_chunks = ckpt->chunk_cnt;
					bitmap_foreach_set(area->use_bitmap, bitmap_size, copy_to_area);
				}
#undef copy_to_area
				num_chunks *= 2;
				area = area->next;
				ckpt = (const struct dymelor_area_checkpoint *)ptr;
			} while(i == ckpt->i);
		}

		while(area != NULL) {
			memset(area->use_bitmap, 0, bitmap_required_size(num_chunks));
			memset(area->core_bitmap, 0, bitmap_required_size(num_chunks));
			area->core_chunks = 0;
			area->alloc_chunks = 0;
			area->last_chunk = 0;
			num_chunks *= 2;
			area = area->next;
		}
	}

	uint_least32_t used_mem = state_ckpt->used_mem;
	ctx->approx_used_mem = state_ckpt->approx_mem;
	if(approximated)
		ctx->used_mem = ctx->approx_used_mem;
	else
		ctx->used_mem = used_mem;

	stats_take(STATS_RESTORE_STATE_SIZE, ctx->used_mem);
	timer_uint t2 = timer_hr_new();
	stats_take(STATS_RESTORE_TIME, t2 - t);

	if(approximated) {
		global_config.restore(current_lp - lps, current_lp->lib_ctx->state_s);
		stats_take(STATS_APPROX_HANDLER_STATE_SIZE, used_mem - ctx->approx_used_mem);
		stats_take(STATS_APPROX_HANDLER_TIME, timer_hr_value(t2));
	}
}
