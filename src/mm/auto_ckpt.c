/**
 * @file mm/auto_ckpt.c
 *
 * @brief Autonomic checkpoint interval selection module
 *
 * The module which attempts to select the best checkpoint interval
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/auto_ckpt.h>

#include <core/init.h>
#include <log/stats.h>
#include <lp/process.h>

#include <math.h>

#define EXP_AVG(f, old_v, sample) 					\
__extension__({								\
	double s = (sample);						\
	double o = (old_v);						\
	o * ((f - 1.0) / f) + s * (1.0 / f);				\
})

static __thread struct {
	double ckpt_avg_cost;
	double inv_sil_avg_cost;
} ackpt;


void auto_ckpt_init(void)
{
	ackpt.ckpt_avg_cost = 4096.0;
	ackpt.inv_sil_avg_cost = 1.0 / 4096.0;
}

/**
 * @brief Compute and set the optimal checkpoint interval for the current worker
 *
 * This function should be called only at the end of GVT reductions, because
 * the used statistics values are representative only in that moment
 */
void auto_ckpt_on_gvt(void)
{
	if (unlikely(global_config.ckpt_interval))
		return;

	uint64_t ckpt_count = stats_retrieve(STATS_CKPT);
	uint64_t ckpt_cost = stats_retrieve(STATS_CKPT_TIME);
	uint64_t sil_count = stats_retrieve(STATS_MSG_SILENT);
	uint64_t sil_cost = stats_retrieve(STATS_MSG_SILENT_TIME);

	if (likely(sil_count))
		ackpt.inv_sil_avg_cost = EXP_AVG(16.0, ackpt.inv_sil_avg_cost,
				(double)sil_count / (double)sil_cost);

	if (likely(ckpt_count))
		ackpt.ckpt_avg_cost = EXP_AVG(16.0, ackpt.ckpt_avg_cost,
				(double)ckpt_cost / (double)ckpt_count);
}

void auto_ckpt_lp_init(struct auto_ckpt *auto_ckpt)
{
	memset(auto_ckpt, 0, sizeof(*auto_ckpt));
	auto_ckpt->ckpt_interval = global_config.ckpt_interval ?
			global_config.ckpt_interval : 256;
	auto_ckpt->inv_bad_p = 64.0;
}

void auto_ckpt_lp_on_gvt(struct auto_ckpt *auto_ckpt)
{
	if (unlikely(!auto_ckpt->m_bad || global_config.ckpt_interval))
		return;

	auto_ckpt->inv_bad_p = EXP_AVG(8.0, auto_ckpt->inv_bad_p,
			2.0 * auto_ckpt->m_good / auto_ckpt->m_bad);
	auto_ckpt->m_bad = 0;
	auto_ckpt->m_good = 0;
	auto_ckpt->ckpt_interval = ceil(sqrt(auto_ckpt->inv_bad_p *
			ackpt.ckpt_avg_cost * ackpt.inv_sil_avg_cost));
}
