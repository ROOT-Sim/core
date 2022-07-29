#include <lib/approximated/approximated.h>

#include <lib/lib.h>
#include <log/stats.h>
#include <lp/lp.h>
#include <mm/auto_ckpt.h>
#include <mm/dymelor/dymelor.h>

#define ALPHA_PREF 1.2

void approximated_lp_on_gvt(struct lp_ctx *ctx)
{
	if (ctx->lib_ctx->approximated_mode != APPROXIMATED_MODE_AUTONOMIC)
		return;

	unsigned approx_ckpt_interval = ctx->auto_ckpt.approx_ckpt_interval;
	unsigned ckpt_interval = ctx->auto_ckpt.ckpt_interval;

	uint_fast32_t state_size = ctx->mm_state.used_mem;
	uint_fast32_t core_state_size = ctx->mm_state.approx_used_mem;

	double rec_cost = ackpt.rec_avg_cost;
	double half_silent_cost = 1 / (2 * ackpt.inv_sil_avg_cost);
	double rb_probability = 1 / ctx->auto_ckpt.inv_bad_p;
	double ckpt_cost = ackpt.ckpt_avg_cost;

	double approx_cost =
	    core_state_size * rec_cost +
	    ackpt.approx_handler_cost * (state_size - core_state_size) +
	    (approx_ckpt_interval - 1) * half_silent_cost;
	approx_cost *= rb_probability;
	approx_cost += core_state_size * ckpt_cost / approx_ckpt_interval;

	double precise_cost =
	    state_size * rec_cost +
	    (ckpt_interval - 1) * half_silent_cost;
	approx_cost *= rb_probability;
	approx_cost += state_size * ckpt_cost / ckpt_interval;

	ctx->mm_state.is_approximated = ALPHA_PREF * approx_cost < precise_cost;
}

void approximated_lp_on_rollback(void)
{
	struct lib_ctx *ctx = current_lp->lib_ctx;
	if (ctx->approximated_mode != APPROXIMATED_MODE_AUTONOMIC)
		current_lp->mm_state.is_approximated = ctx->approximated_mode == APPROXIMATED_MODE_APPROXIMATED;
}

void approximated_lp_init(void)
{
	ApproximatedModeSwitch(APPROXIMATED_MODE_PRECISE);
}

void ApproximatedModeSwitch(enum approximated_mode mode)
{
	struct lib_ctx *ctx = current_lp->lib_ctx;
	ctx->approximated_mode = mode;
	if (mode != APPROXIMATED_MODE_AUTONOMIC)
		current_lp->mm_state.is_approximated = mode == APPROXIMATED_MODE_APPROXIMATED;
}

void ApproximatedMemoryMark(const void *base, bool core)
{
	// FIXME: check base belongs to the LP memory allocator
	const unsigned char *p = base;
	struct dymelor_area *m_area = (struct dymelor_area *)(p - *(uint_least32_t *)(p - sizeof(uint_least32_t)));
	uint_least32_t i = (p - m_area->area) >> m_area->chk_size_exp;
	if(bitmap_check(m_area->core_bitmap, i) != core) {
		if (core) {
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
