/**
 * @file mm/dymelor/dymelor.h
 *
 * @brief Dynamic Memory Logger and Restorer Library
 *
 * Implementation of the DyMeLoR memory allocator
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/dymelor/dymelor.h>

#include <lp/lp.h>
#include <mm/dymelor/checkpoint.h>
#include <mm/mm.h>

#include <errno.h>

#define next_exp_of_2(i) (sizeof(i) * CHAR_BIT - intrinsics_clz(i))

void dymelor_lp_init(struct dymelor_state *self)
{
	memset(self->areas, 0, sizeof(self->areas));
	self->used_mem = 0;
	array_init(self->logs);
}

void dymelor_lp_fini(struct dymelor_state *self)
{
	for(array_count_t i = 0; i < array_count(self->logs); ++i)
		mm_free(array_get_at(self->logs, i).c);

	array_fini(self->logs);

	for(unsigned i = 0; i < NUM_AREAS; ++i) {
		struct dymelor_area *area = self->areas[i];
		while(area != NULL) {
			struct dymelor_area *next = area->next;
			mm_free(area);
			area = next;
		}
	}
}

void dymelor_checkpoint_take(struct dymelor_state *self, array_count_t ref_i)
{
	struct dymelor_log mm_log = {.ref_i = ref_i, .c = dymelor_checkpoint_full_take(self)};
	array_push(self->logs, mm_log);
}

void dymelor_checkpoint_next_force_full(struct dymelor_state *self)
{
	(void)self;
	// todo: support incremental checkpointing properly
}

array_count_t dymelor_checkpoint_restore(struct dymelor_state *self, array_count_t ref_i)
{
	array_count_t i = array_count(self->logs) - 1;
	while(array_get_at(self->logs, i).ref_i > ref_i)
		i--;

	dymelor_checkpoint_full_restore(self, array_get_at(self->logs, i).c);

	for(array_count_t j = array_count(self->logs) - 1; j > i; --j)
		mm_free(array_get_at(self->logs, j).c);

	array_count(self->logs) = i + 1;
	return array_get_at(self->logs, i).ref_i;
}

array_count_t dymelor_fossil_lp_collect(struct dymelor_state *self, array_count_t tgt_ref_i)
{
	array_count_t log_i = array_count(self->logs) - 1;
	array_count_t ref_i = array_get_at(self->logs, log_i).ref_i;
	while(ref_i > tgt_ref_i) {
		--log_i;
		ref_i = array_get_at(self->logs, log_i).ref_i;
	}

	array_count_t j = array_count(self->logs);
	while(j > log_i) {
		--j;
		array_get_at(self->logs, j).ref_i -= ref_i;
	}

	while(j--)
		mm_free(array_get_at(self->logs, j).c);

	array_truncate_first(self->logs, log_i);
	return ref_i;
}

static struct dymelor_area *malloc_area_new(unsigned chunk_size_exp, uint_least32_t num_chunks)
{
	size_t bitmap_size = bitmap_required_size(num_chunks);
	struct dymelor_area *area =
	    mm_alloc(offsetof(struct dymelor_area, area) + 2 * bitmap_size + (num_chunks << chunk_size_exp));
	area->alloc_chunks = 0;
	area->dirty_chunks = 0;
	area->last_chunk = 0;
	area->chk_size_exp = chunk_size_exp;
	area->use_bitmap = area->area + (num_chunks << chunk_size_exp);
	area->dirty_bitmap = area->use_bitmap + bitmap_size;
	memset(area->use_bitmap, 0, 2 * bitmap_size);
	area->next = NULL;
	return area;
}

void *dymelor_alloc(struct dymelor_state *self, size_t req_size)
{
	if(unlikely(req_size > MAX_CHUNK_SIZE)) {
		logger(LOG_ERROR,
		    "Requested a memory allocation of %d but the limit is %d. Reconfigure MAX_CHUNK_SIZE. Returning NULL",
		    req_size, MAX_CHUNK_SIZE);
		return NULL;
	}

	req_size += sizeof(uint_least32_t);
	unsigned size_exp = max(next_exp_of_2(req_size), MIN_CHUNK_EXP);

	uint_least32_t num_chunks = MIN_NUM_CHUNKS;
	struct dymelor_area **m_area_p = &self->areas[size_exp - MIN_CHUNK_EXP];
	struct dymelor_area *m_area = *m_area_p;
	while(m_area != NULL && m_area->alloc_chunks >= num_chunks) {
		m_area_p = &m_area->next;
		m_area = *m_area_p;
		num_chunks *= 2;
	}

	if(unlikely(m_area == NULL)) {
		*m_area_p = malloc_area_new(size_exp, num_chunks);
		m_area = *m_area_p;
	}

	while(bitmap_check(m_area->use_bitmap, m_area->last_chunk))
		m_area->last_chunk++;

	bitmap_set(m_area->use_bitmap, m_area->last_chunk);
	++m_area->alloc_chunks;
	self->used_mem += 1U << size_exp;
	uint_least32_t offset = m_area->last_chunk << size_exp;
	m_area->last_chunk++;

	unsigned char *ptr = m_area->area + offset;
	*(uint_least32_t *)(ptr - sizeof(uint_least32_t)) = offset + offsetof(struct dymelor_area, area);
	return ptr;
}

void *dymelor_realloc(struct dymelor_state *self, void *ptr, size_t req_size)
{
	if(!req_size) { // Adhering to C11 standard §7.20.3.1
		if(!ptr)
			errno = EINVAL;
		return NULL;
	}
	if(!ptr)
		return rs_malloc(req_size);

	unsigned char *p = ptr;
	struct dymelor_area *m_area = (struct dymelor_area *)(p - *(uint_least32_t *)(p - sizeof(uint_least32_t)));
	void *new_buffer = dymelor_alloc(self, req_size);
	if(unlikely(new_buffer == NULL))
		return NULL;

	memcpy(new_buffer, ptr, min(req_size, (1U << m_area->chk_size_exp) - sizeof(uint_least32_t)));
	rs_free(ptr);

	return new_buffer;
}

void dymelor_free(struct dymelor_state *self, void *ptr)
{
	unsigned char *p = ptr;
	struct dymelor_area *m_area = (struct dymelor_area *)(p - *(uint_least32_t *)(p - sizeof(uint_least32_t)));

	uint_least32_t idx = (p - m_area->area) >> m_area->chk_size_exp;
	if(!bitmap_check(m_area->use_bitmap, idx)) {
		logger(LOG_FATAL, "double free() corruption or address not malloc'd\n");
		abort();
	}
	bitmap_reset(m_area->use_bitmap, idx);
	--m_area->alloc_chunks;
	self->used_mem -= 1U << m_area->chk_size_exp;

#ifdef ROOTSIM_INCREMENTAL
	if(bitmap_check(m_area->dirty_bitmap, idx)) {
		bitmap_reset(m_area->dirty_bitmap, idx);
		m_area->dirty_chunks--;
	}
#endif

	if(idx < m_area->last_chunk)
		m_area->last_chunk = idx;
}

void dymelor_dirty_mark(struct dymelor_state *self, void *base, int size)
{
	(void)self, (void)size;
	// FIXME: check base belongs to the LP memory allocator; doing this efficiently requires DyMeLoR memory areas to
	//        come from the same underlying allocator (maybe use a large grained buddy system underneath)
	unsigned char *p = base;
	struct dymelor_area *m_area = (struct dymelor_area *)(p - *(uint_least32_t *)(p - sizeof(uint_least32_t)));
	uint_least32_t i = (p - m_area->area) >> m_area->chk_size_exp;
	if(!bitmap_check(m_area->dirty_bitmap, i)) {
		bitmap_set(m_area->dirty_bitmap, i);
		m_area->dirty_chunks++;
	}
}
