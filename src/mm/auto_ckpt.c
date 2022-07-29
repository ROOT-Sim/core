/**
 * @file mm/auto_ckpt.c
 *
 * @brief Autonomic checkpoint interval selection module
 *
 * The module which attempts to select the best checkpoint interval
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <mm/auto_ckpt.h>

#include <log/stats.h>
#include <lp/process.h>

#include <math.h>

__thread struct auto_ckpt_ctx ackpt;

/**
 * @brief Initialize the thread-local context for the auto checkpoint module
 */
void auto_ckpt_init(void)
{
	ackpt.ckpt_avg_cost = 1.0;
	ackpt.rec_avg_cost = 1.0;
	ackpt.approx_handler_cost = 1.0;
	ackpt.inv_sil_avg_cost = 1.0 / 4096.0;
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
	uint64_t ckpt_state_size = stats_retrieve(STATS_CKPT_STATE_SIZE);
	uint64_t sil_count = stats_retrieve(STATS_MSG_SILENT);
	uint64_t sil_cost = stats_retrieve(STATS_MSG_SILENT_TIME);
	uint64_t recovery_state_size = stats_retrieve(STATS_RESTORE_STATE_SIZE);
	uint64_t recovery_cost = stats_retrieve(STATS_RESTORE_TIME);
	uint64_t approx_handler_cost = stats_retrieve(STATS_APPROX_HANDLER_TIME);
	uint64_t approx_handler_state_size = stats_retrieve(STATS_APPROX_HANDLER_STATE_SIZE);

	if(likely(sil_count))
		ackpt.inv_sil_avg_cost = EXP_AVG(16.0, ackpt.inv_sil_avg_cost, (double)sil_count / (double)sil_cost);

	if(likely(ckpt_state_size))
		ackpt.ckpt_avg_cost = EXP_AVG(16.0, ackpt.ckpt_avg_cost, (double)ckpt_cost / (double)ckpt_state_size);

	if(likely(recovery_state_size))
		ackpt.rec_avg_cost = EXP_AVG(16.0, ackpt.rec_avg_cost, (double)recovery_cost / (double)recovery_state_size);

	if(likely(approx_handler_state_size))
		ackpt.approx_handler_cost = EXP_AVG(16.0, ackpt.approx_handler_cost, (double)approx_handler_cost / (double)approx_handler_state_size);
}

/**
 * @brief Initialize the per-LP context for the auto checkpoint module
 * @param auto_ckpt a pointer to the LP auto checkpoint context to initialize
 */
void auto_ckpt_lp_init(struct auto_ckpt *auto_ckpt)
{
	memset(auto_ckpt, 0, sizeof(*auto_ckpt));
	auto_ckpt->ckpt_interval = global_config.ckpt_interval ? global_config.ckpt_interval : 256;
	auto_ckpt->approx_ckpt_interval = global_config.ckpt_interval ? global_config.ckpt_interval : 256;
	auto_ckpt->inv_bad_p = 64.0;
}

/**
 * @brief Compute the optimal checkpoint interval of the current LP and set it
 * @param auto_ckpt a pointer to the auto checkpoint context of the current LP
 * @param state_size the size in bytes of the checkpoint-able state of the current LP
 *
 * This function should be called only at the end of GVT reductions, because
 * the used statistics values are representative only in that moment.
 */
void auto_ckpt_lp_on_gvt(struct auto_ckpt *auto_ckpt, uint_fast32_t state_size, uint_fast32_t approx_state_size)
{
	if(unlikely(!auto_ckpt->m_bad || global_config.ckpt_interval))
		return;

	auto_ckpt->inv_bad_p = EXP_AVG(8.0, auto_ckpt->inv_bad_p, auto_ckpt->m_good / auto_ckpt->m_bad);
	auto_ckpt->m_bad = 0;
	auto_ckpt->m_good = 0;
	auto_ckpt->ckpt_interval =
	    ceil(sqrt(2.0 * auto_ckpt->inv_bad_p * ackpt.ckpt_avg_cost * ackpt.inv_sil_avg_cost * (double)state_size));
	auto_ckpt->approx_ckpt_interval =
	    ceil(sqrt(2.0 * auto_ckpt->inv_bad_p * ackpt.ckpt_avg_cost * ackpt.inv_sil_avg_cost * (double)approx_state_size));
}
