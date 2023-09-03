/**
 * @file mm/auto_ckpt.c
 *
 * @brief Autonomic checkpoint interval selection module
 *
 * The module which attempts to select the best checkpoint interval
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/auto_ckpt.h>

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
		o *(((f)-1.0f) / (f)) + s * (1.0f / (f));                                                              \
	})

static __thread struct {
	float ckpt_avg_cost;
	float inv_sil_avg_cost;
} ackpt;

/**
 * @brief Initialize the thread-local context for the auto checkpoint module
 */
void auto_ckpt_init(void)
{
	ackpt.ckpt_avg_cost = 1.0f;
	ackpt.inv_sil_avg_cost = 1.0f / 4096.0f;
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

	uint64_t ckpt_cost = stats_retrieve(STATS_CKPT_TIME);
	uint64_t ckpt_size = stats_retrieve(STATS_CKPT_SIZE);
	uint64_t sil_count = stats_retrieve(STATS_MSG_SILENT);
	uint64_t sil_cost = stats_retrieve(STATS_MSG_SILENT_TIME);

	if(likely(sil_count))
		ackpt.inv_sil_avg_cost = EXP_AVG(16.0f, ackpt.inv_sil_avg_cost, (float)sil_count / (float)sil_cost);

	if(likely(ckpt_size))
		ackpt.ckpt_avg_cost = EXP_AVG(16.0f, ackpt.ckpt_avg_cost, (float)ckpt_cost / (float)ckpt_size);
}

/**
 * @brief Initialize the per-LP context for the auto checkpoint module
 * @param auto_ckpt a pointer to the LP auto checkpoint context to initialize
 */
void auto_ckpt_lp_init(struct auto_ckpt *auto_ckpt)
{
	memset(auto_ckpt, 0, sizeof(*auto_ckpt));
	auto_ckpt->ckpt_interval = global_config.ckpt_interval ? global_config.ckpt_interval : 256;
	auto_ckpt->inv_bad_p = 64.0f;
}

/**
 * @brief Compute the optimal checkpoint interval of the current LP and set it
 * @param auto_ckpt a pointer to the auto checkpoint context of the current LP
 * @param state_size the size in bytes of the checkpoint-able state of the current LP
 */
void auto_ckpt_recompute(struct auto_ckpt *auto_ckpt, uint_fast32_t state_size)
{
	if(unlikely(!auto_ckpt->m_bad || global_config.ckpt_interval))
		return;

	auto_ckpt->inv_bad_p = EXP_AVG(8.0f, auto_ckpt->inv_bad_p, 2.0f * auto_ckpt->m_good / auto_ckpt->m_bad);
	auto_ckpt->m_bad = 0;
	auto_ckpt->m_good = 0;
	auto_ckpt->ckpt_interval = (unsigned short)ceilf(
	    sqrtf(auto_ckpt->inv_bad_p * ackpt.ckpt_avg_cost * ackpt.inv_sil_avg_cost * (float)state_size));
}
