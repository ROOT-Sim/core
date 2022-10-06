#include <mm/dymelor/checkpoint.h>

#include <core/core.h>
#include <mm/dymelor/dymelor.h>
#include <mm/mm.h>

static size_t compute_log_size(const struct dymelor_state *ctx)
{
	size_t ret = sizeof(offsetof(struct dymelor_ctx_checkpoint, data));
	for(unsigned i = 0; i < NUM_AREAS; ++i) {
		const struct dymelor_area *area = ctx->areas[i];
		uint_least32_t num_chunks = MIN_NUM_CHUNKS;
		uint_least32_t chunk_size = (1U << (MIN_CHUNK_EXP + i)) - sizeof(uint_least32_t);
		while(area != NULL) {
			ret += sizeof(offsetof(struct dymelor_area_checkpoint, data));
			ret += bitmap_required_size(num_chunks);
#ifdef ROOTSIM_INCREMENTAL
			ret += bitmap_required_size(num_chunks); // maybe not needed in full checkpoints
#endif
			ret += chunk_size * area->alloc_chunks;
			num_chunks *= 2;
			area = area->next;
		}
	}
	return ret;
}

struct dymelor_ctx_checkpoint *dymelor_checkpoint_full_take(const struct dymelor_state *ctx)
{
	struct dymelor_ctx_checkpoint *ckpt = mm_alloc(compute_log_size(ctx));
	unsigned char * restrict ptr = ckpt->data;
	ckpt->area_cnt = 0;
	for(unsigned i = 0; i < NUM_AREAS; ++i) {
		const struct dymelor_area *area = ctx->areas[i];
		uint_least32_t num_chunks = MIN_NUM_CHUNKS;
		uint_least32_t chunk_size = (1U << (MIN_CHUNK_EXP + i)) - sizeof(uint_least32_t);
		while(area != NULL) {
			struct dymelor_area_checkpoint *ackpt = (struct dymelor_area_checkpoint *)ptr;
			ackpt->i = i;
			ackpt->chunk_cnt = area->alloc_chunks;
			size_t bitmap_size = bitmap_required_size(num_chunks);
			memcpy(ackpt->data, area->use_bitmap, bitmap_size);
			ptr = ackpt->data + bitmap_size;

#define copy_from_area(x)                                                                                              \
	({                                                                                                             \
		memcpy(ptr, area->area + ((x) * (chunk_size + sizeof(uint_least32_t))), chunk_size);                   \
		ptr += chunk_size;                                                                                     \
	})

			// Copy only the allocated chunks
			bitmap_foreach_set(area->use_bitmap, bitmap_size, copy_from_area);

#undef copy_from_area

			++ckpt->area_cnt;
			num_chunks *= 2;
			area = area->next;
		}
	}

	return ckpt;
}

void dymelor_checkpoint_full_restore(struct dymelor_state *ctx, const struct dymelor_ctx_checkpoint *ckpt)
{
	const unsigned char *ptr = ckpt->data;
	unsigned last_i = UINT_MAX;
	struct dymelor_area *area = NULL;
	uint_least32_t num_chunks, chunk_size;
	unsigned j = ckpt->area_cnt;
	while(j--) {
		struct dymelor_area_checkpoint *ackpt = (struct dymelor_area_checkpoint *)ptr;
		if(last_i != ackpt->i) {
			if (area != NULL)
				while(unlikely(area->next != NULL)) {
					area = area->next;
					num_chunks *= 2;
					memset(area->use_bitmap, 0, bitmap_required_size(num_chunks));
					area->alloc_chunks = 0;
				}

			num_chunks = MIN_NUM_CHUNKS;
			last_i = ackpt->i;
			chunk_size = (1U << (MIN_CHUNK_EXP + last_i)) - sizeof(uint_least32_t);
			area = ctx->areas[last_i];
		} else {
			num_chunks *= 2;
			area = area->next;
		}

		area->alloc_chunks = ackpt->chunk_cnt;
		size_t bitmap_size = bitmap_required_size(num_chunks);
		memcpy(area->use_bitmap, ackpt->data, bitmap_size);
		ptr = ackpt->data + bitmap_size;

#define copy_to_area(x)                                                                                                \
	({                                                                                                             \
		memcpy(area->area + ((x) * (chunk_size + sizeof(uint_least32_t))), ptr, chunk_size);                   \
		ptr += chunk_size;                                                                                     \
	})

		bitmap_foreach_set(area->use_bitmap, bitmap_size, copy_to_area);

#undef copy_to_area
	}
}
