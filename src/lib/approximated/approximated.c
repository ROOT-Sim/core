#include <lib/approximated/approximated.h>

#include <lib/lib.h>
#include <lp/lp.h>
#include <mm/dymelor/dymelor.h>


void approximated_lp_on_gvt(struct lp_ctx *ctx)
{
	if (ctx->lib_ctx->approximated_mode != APPROXIMATED_MODE_AUTONOMIC)
		return;

	// todo compute autonomic policy
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
