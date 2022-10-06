#include <mm/model_allocator.h>

#include <arch/timer.h>
#include <log/stats.h>
#include <lp/lp.h>

#include <errno.h>

void model_allocator_lp_init(struct mm_state *self)
{
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			multi_buddy_lp_init(&self->m_mb);
			break;
		case MM_DYMELOR:
			dymelor_lp_init(&self->m_dy);
			break;
		default:
			assert(0);
			__builtin_unreachable();
	}
}

void model_allocator_lp_fini(struct mm_state *self)
{
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			multi_buddy_lp_fini(&self->m_mb);
			break;
		case MM_DYMELOR:
			dymelor_lp_fini(&self->m_dy);
			break;
		default:
			assert(0);
			__builtin_unreachable();
	}
}

void model_allocator_checkpoint_take(struct mm_state *self, array_count_t ref_i)
{
	timer_uint t = timer_hr_new();
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			multi_buddy_checkpoint_take(&self->m_mb, ref_i);
			stats_take(STATS_CKPT_STATE_SIZE, self->m_mb.used_mem);
			break;
		case MM_DYMELOR:
			dymelor_checkpoint_take(&self->m_dy, ref_i);
			stats_take(STATS_CKPT_STATE_SIZE, self->m_dy.used_mem);
			break;
		default:
			assert(0);
			__builtin_unreachable();
	}
	stats_take(STATS_CKPT, 1);
	stats_take(STATS_CKPT_TIME, timer_hr_value(t));
}

void model_allocator_checkpoint_next_force_full(struct mm_state *self)
{
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			multi_buddy_checkpoint_next_force_full(&self->m_mb);
			break;
		case MM_DYMELOR:
			dymelor_checkpoint_next_force_full(&self->m_dy);
			break;
		default:
			assert(0);
			__builtin_unreachable();
	}
}

array_count_t model_allocator_checkpoint_restore(struct mm_state *self, array_count_t ref_i)
{
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			return multi_buddy_checkpoint_restore(&self->m_mb, ref_i);
		case MM_DYMELOR:
			return dymelor_checkpoint_restore(&self->m_dy, ref_i);
		default:
			assert(0);
			__builtin_unreachable();
	}
}

array_count_t model_allocator_fossil_lp_collect(struct mm_state *self, array_count_t tgt_ref_i)
{
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			return multi_buddy_fossil_lp_collect(&self->m_mb, tgt_ref_i);
		case MM_DYMELOR:
			return dymelor_fossil_lp_collect(&self->m_dy, tgt_ref_i);
		default:
			assert(0);
			__builtin_unreachable();
	}
}

void *rs_malloc(size_t req_size)
{
	if(unlikely(!req_size))
		return NULL;

	struct mm_state *self = &current_lp->mm_state;
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			return multi_buddy_alloc(&self->m_mb, req_size);
		case MM_DYMELOR:
			return dymelor_alloc(&self->m_dy, req_size);
		default:
			assert(0);
			__builtin_unreachable();
	}
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
		if(unlikely(!ptr))
			errno = EINVAL;
		return NULL;
	}

	if(!ptr)
		return rs_malloc(req_size);

	struct mm_state *self = &current_lp->mm_state;
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			return multi_buddy_realloc(&self->m_mb, ptr, req_size);
		case MM_DYMELOR:
			return dymelor_realloc(&self->m_dy, ptr, req_size);
		default:
			assert(0);
			__builtin_unreachable();
	}
}

void rs_free(void *ptr)
{
	if(unlikely(ptr == NULL))
		return;

	struct mm_state *self = &current_lp->mm_state;
	switch(global_config.mm) {
		case MM_MULTI_BUDDY:
			multi_buddy_free(&self->m_mb, ptr);
			break;
		case MM_DYMELOR:
			dymelor_free(&self->m_dy, ptr);
			break;
		default:
			assert(0);
			__builtin_unreachable();
	}
}
