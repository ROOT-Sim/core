/**
 * @file mm/checkpoint/autonomic.c
 *
 * @brief Autonomic checkpoint interval selection module
 *
 * The module which attempts to select the best checkpoint interval
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/checkpoint/checkpoint.h>

#include <log/stats.h>
#include <lp/process.h>

#include <math.h>

/**
 * Compute a new value of the exponential moving average
 * @param f the retention factor for old observations
 * @param old_v the latest value of the moving average
 * @param sample the new value to include in the average
 * @return the new value of the exponential moving average
 */
#define EXP_AVG(f, old_v, sample)                                                                                      \
	__extension__({                                                                                                \
		float s = (sample);                                                                                    \
		float o = (old_v);                                                                                     \
		(((f) - 1.0f) / (f)) * o + (1.0f / (f)) * s;                                                           \
	})

/**
 * @brief Thread-local context for the auto checkpoint module
 *
 * This structure holds thread-local metrics used for computing
 * the optimal checkpoint interval.
 */
static _Thread_local struct {
	float ckpt_avg_cost;    /**< Exponential moving average of checkpoint cost per byte */
	float inv_sil_avg_cost; /**< Exponential moving average of the inverse of a single silent message cost */
} auto_ckpt_thread_ctx;

/**
 * @brief Initialize the thread-local context for the auto checkpoint module
 */
void auto_ckpt_init(void)
{
	auto_ckpt_thread_ctx.ckpt_avg_cost = 1.0f;
	auto_ckpt_thread_ctx.inv_sil_avg_cost = 1.0f / 4096.0f;
}

/**
 * @brief Compute the thread-local metrics needed for the per-LP computations
 *
 * This function should be called only at the end of GVT reductions, because
 * the used statistics values are representative only in that moment.
 */
void auto_ckpt_on_gvt(void)
{
	if(unlikely(global_config.ckpt_interval))
		return;

	const uint64_t ckpt_cost = stats_retrieve(STATS_CKPT_TIME);
	const uint64_t ckpt_size = stats_retrieve(STATS_CKPT_SIZE);
	const uint64_t sil_count = stats_retrieve(STATS_MSG_SILENT);
	const uint64_t sil_cost = stats_retrieve(STATS_MSG_SILENT_TIME);

	if(likely(sil_cost))
		auto_ckpt_thread_ctx.inv_sil_avg_cost =
		    EXP_AVG(16.0f, auto_ckpt_thread_ctx.inv_sil_avg_cost, (float)(sil_count / (double)sil_cost));

	if(likely(ckpt_size))
		auto_ckpt_thread_ctx.ckpt_avg_cost =
		    EXP_AVG(16.0f, auto_ckpt_thread_ctx.ckpt_avg_cost, (float)(ckpt_cost / (double)ckpt_size));
}

/**
 * @brief Initialize the per-LP context for the auto checkpoint module
 * @param auto_ckpt a pointer to the LP auto checkpoint context to initialize
 */
void auto_ckpt_lp_init(struct auto_ckpt *auto_ckpt)
{
	auto_ckpt->inv_bad_p = 64.0f;
	auto_ckpt->ckpt_interval = global_config.ckpt_interval ? global_config.ckpt_interval : 256;
	auto_ckpt->ckpt_rem = auto_ckpt->ckpt_interval; // forces a checkpoint after the first event
	auto_ckpt->m_bad = 0;
	auto_ckpt->m_good = 0;
}

/**
 * @brief Compute the optimal checkpoint interval of the current LP and set it
 * @param auto_ckpt a pointer to the auto checkpoint context of the current LP
 * @param state_size the size in bytes of the checkpoint-able state of the current LP
 */
void auto_ckpt_recompute(struct auto_ckpt *auto_ckpt, const uint_fast32_t state_size)
{
	if(unlikely(!auto_ckpt->m_bad || global_config.ckpt_interval))
		return;

	auto_ckpt->inv_bad_p = EXP_AVG(8.0f, auto_ckpt->inv_bad_p, 2.0f * auto_ckpt->m_good / auto_ckpt->m_bad);
	auto_ckpt->m_bad = 0;
	auto_ckpt->m_good = 0;
	auto_ckpt->ckpt_interval =
	    (unsigned short)ceilf(sqrtf(auto_ckpt->inv_bad_p * auto_ckpt_thread_ctx.ckpt_avg_cost *
					auto_ckpt_thread_ctx.inv_sil_avg_cost * (float)state_size));
}
