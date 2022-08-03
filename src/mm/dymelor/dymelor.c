#include <mm/dymelor/dymelor.h>

#include <lp/lp.h>
#include <mm/dymelor/checkpoint.h>
#include <mm/mm.h>

#include <errno.h>

#define next_exp_of_2(i) (sizeof(i) * CHAR_BIT - intrinsics_clz(i))

void model_allocator_lp_init(void)
{
	struct mm_state *self = &current_lp->mm_state;
	memset(self->areas, 0, sizeof(self->areas));
	array_init(self->logs);
	self->is_approximated = false;
	self->used_mem = 0;
	self->approx_used_mem = 0;
}

void model_allocator_lp_fini(void)
{
	struct mm_state *self = &current_lp->mm_state;

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

void model_allocator_checkpoint_take(array_count_t ref_i)
{
	struct mm_state *self = &current_lp->mm_state;
	struct dymelor_log mm_log = {.ref_i = ref_i, .c = checkpoint_full_take(self, self->is_approximated)};
	array_push(self->logs, mm_log);
}

void model_allocator_checkpoint_next_force_full(void)
{
	// todo: support incremental checkpointing properly
}

array_count_t model_allocator_checkpoint_restore(array_count_t ref_i)
{
	struct mm_state *self = &current_lp->mm_state;
	array_count_t i = array_count(self->logs) - 1;
	while(array_get_at(self->logs, i).ref_i > ref_i)
		i--;

	checkpoint_full_restore(self, array_get_at(self->logs, i).c);

	for(array_count_t j = array_count(self->logs) - 1; j > i; --j)
		mm_free(array_get_at(self->logs, j).c);

	array_count(self->logs) = i + 1;
	return array_get_at(self->logs, i).ref_i;
}

array_count_t model_allocator_fossil_lp_collect(struct mm_state *self, array_count_t tgt_ref_i)
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
	size_t bitmap_size = 2 * bitmap_required_size(num_chunks);
#ifdef ROOTSIM_INCREMENTAL
	bitmap_size += bitmap_required_size(num_chunks);
#endif
	struct dymelor_area *area =
	    mm_alloc(offsetof(struct dymelor_area, area) + bitmap_size + (num_chunks << chunk_size_exp));
	area->alloc_chunks = 0;
	area->core_chunks = 0;
	area->last_chunk = 0;
	area->last_access = -1;
	area->chk_size_exp = chunk_size_exp;
	area->use_bitmap = area->area + (num_chunks << chunk_size_exp);
	area->core_bitmap = area->use_bitmap + bitmap_required_size(num_chunks);
#ifdef ROOTSIM_INCREMENTAL
	area->dirty_bitmap = area->approx_bitmap + bitmap_required_size(num_chunks);
	area->dirty_chunks = 0;
#endif
	memset(area->use_bitmap, 0, bitmap_size);
	area->next = NULL;
	return area;
}

void *rs_malloc(size_t req_size)
{
	if(unlikely(req_size == 0))
		return NULL;

	if(unlikely(req_size > MAX_CHUNK_SIZE)) {
		logger(LOG_ERROR,
		    "Requested a memory allocation of %d but the limit is %d. Reconfigure MAX_CHUNK_SIZE. Returning NULL",
		    req_size, MAX_CHUNK_SIZE);
		return NULL;
	}

	req_size += sizeof(uint_least32_t);
	unsigned size_exp = max(next_exp_of_2(req_size), MIN_CHUNK_EXP);
	current_lp->mm_state.used_mem += 1 << size_exp;
	current_lp->mm_state.approx_used_mem += 1 << size_exp;

	uint_least32_t num_chunks = MIN_NUM_CHUNKS;
	struct dymelor_area **m_area_p = &current_lp->mm_state.areas[size_exp - MIN_CHUNK_EXP];
	struct dymelor_area *m_area = *m_area_p;
	while(m_area != NULL && m_area->alloc_chunks >= num_chunks) {
		m_area_p = &m_area->next;
		m_area = *m_area_p;
		num_chunks *= 2;
	}

	if(unlikely(m_area == NULL)) {
		// Allocate a new malloc area
		*m_area_p = malloc_area_new(size_exp, num_chunks);
		m_area = *m_area_p;
	}

	while(bitmap_check(m_area->use_bitmap, m_area->last_chunk))
		m_area->last_chunk++;

	bitmap_set(m_area->use_bitmap, m_area->last_chunk);
	bitmap_set(m_area->core_bitmap, m_area->last_chunk);
	// m_area->last_access = lvt(current);
	++m_area->alloc_chunks;
	++m_area->core_chunks;
	uint_least32_t offset = m_area->last_chunk << size_exp;
	m_area->last_chunk++;

	unsigned char *ptr = m_area->area + offset;
	*(uint_least32_t *)(ptr - sizeof(uint_least32_t)) = offset + offsetof(struct dymelor_area, area);
	return ptr;
}

void *rs_calloc(size_t nmemb, size_t size)
{
	size_t tot = nmemb * size;
	void *ret = rs_malloc(tot);

	if(likely(ret))
		memset(ret, 0, tot);

	return ret;
}

void *rs_realloc(void *ptr, size_t req_size)
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
	void *new_buffer = rs_malloc(req_size);
	if(unlikely(new_buffer == NULL))
		return NULL;

	memcpy(new_buffer, ptr, min(req_size, (1U << m_area->chk_size_exp) - sizeof(uint_least32_t)));
	rs_free(ptr);

	return new_buffer;
}

void rs_free(void *ptr)
{
	if(unlikely(ptr == NULL))
		return;

	unsigned char *p = ptr;
	struct dymelor_area *m_area = (struct dymelor_area *)(p - *(uint_least32_t *)(p - sizeof(uint_least32_t)));

	uint_least32_t idx = (p - m_area->area) >> m_area->chk_size_exp;
#ifndef NDEBUG
	if(!bitmap_check(m_area->use_bitmap, idx)) {
		logger(LOG_FATAL, "double free() corruption or address not malloc'd\n");
		abort();
	}
#endif
	bitmap_reset(m_area->use_bitmap, idx);
	--m_area->alloc_chunks;

	if(bitmap_check(m_area->core_bitmap, idx))
		--m_area->core_chunks;
	bitmap_reset(m_area->core_bitmap, idx);

#ifdef ROOTSIM_INCREMENTAL
	if(bitmap_check(m_area->dirty_bitmap, idx)) {
		bitmap_reset(m_area->dirty_bitmap, idx);
		m_area->dirty_chunks--;
	}
#endif

	// m_area->last_access = lvt(current);

	if(idx < m_area->last_chunk)
		m_area->last_chunk = idx;
}

#ifdef ROOTSIM_INCREMENTAL
void dirty_mem(void *base, int size)
{
	// FIXME: check base belongs to the LP memory allocator
	unsigned char *p = base;
	struct dymelor_area *m_area = (struct dymelor_area *)(p - *(uint_least32_t *)(p - sizeof(uint_least32_t)));
	uint_least32_t i = (p - m_area->area) >> m_area->chk_size_exp;
	if(!bitmap_check(m_area->dirty_bitmap, i)) {
		bitmap_set(m_area->dirty_bitmap, i);
		m_area->dirty_chunks++;
	}
}
#endif

void clean_buffers_on_gvt(struct mm_state *state, simtime_t time_barrier)
{
	// The first NUM_AREAS malloc_areas are placed according to their chunks' sizes. The exceeding malloc_areas can
	// be compacted
	for(unsigned i = 0; i < NUM_AREAS; ++i) {
		struct dymelor_area *m_area = state->areas[i];
		if(m_area == NULL)
			continue;

		struct dymelor_area **m_area_p = &state->areas[i];

		while(m_area->next != NULL) {
			m_area_p = &m_area->next;
			m_area = *m_area_p;
		}

		// todo: free this stuff if empty and safe to delete; requires last_access timestamps
	}
}

void ApproximatedMemoryMark(const void *base, bool is_core)
{
	// FIXME: check base belongs to the LP memory allocator
	const unsigned char *p = base;
	struct dymelor_area *m_area = (struct dymelor_area *)(p - *(uint_least32_t *)(p - sizeof(uint_least32_t)));
	uint_least32_t i = (p - m_area->area) >> m_area->chk_size_exp;
	if(bitmap_check(m_area->core_bitmap, i) != is_core) {
		if (is_core) {
			bitmap_set(m_area->core_bitmap, i);
			m_area->core_chunks++;
			current_lp->mm_state.approx_used_mem += 1 << m_area->chk_size_exp;
		} else {
			bitmap_reset(m_area->core_bitmap, i);
			m_area->core_chunks--;
			current_lp->mm_state.approx_used_mem -= 1 << m_area->chk_size_exp;
		}
	}
}
