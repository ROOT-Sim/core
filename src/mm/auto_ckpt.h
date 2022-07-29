/**
 * @file mm/auto_ckpt.h
 *
 * @brief Autonomic checkpoint interval selection header
 *
 * The module which attempts to select the best checkpoint interval
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <inttypes.h>

/**
 * Compute a new value of the exponential moving average
 * @param f the retention factor for old observations
 * @param old_v the latest value of the moving average
 * @param sample the new value to include in the average
 * @return the new value of the exponential moving average
 */
#define EXP_AVG(f, old_v, sample)                                                                                      \
	__extension__({                                                                                                \
		double s = (sample);                                                                                   \
		double o = (old_v);                                                                                    \
		o *(((f)-1.0) / (f)) + s *(1.0 / (f));                                                                 \
	})

struct auto_ckpt_ctx {
	double ckpt_avg_cost;
	double inv_sil_avg_cost;
	double rec_avg_cost;
	double approx_handler_cost;
};

extern __thread struct auto_ckpt_ctx ackpt;

/// Structure to keep data used for autonomic checkpointing selection
struct auto_ckpt {
	/// The inverse of the rollback probability
	double inv_bad_p;
	/// The count of straggler and anti-messages
	unsigned m_bad;
	/// The count of correctly processed forward messages
	unsigned m_good;
	unsigned approx_ckpt_interval;
	unsigned ckpt_interval;
	/// The count of remaining events to process until the next checkpoint
	unsigned ckpt_rem;
};

/**
 * Register a "bad" message, i.e. one message which cause a rollback
 * @param auto_ckpt a pointer to the auto-checkpoint module struct of the current LP
 */
#define auto_ckpt_register_bad(auto_ckpt) ((auto_ckpt)->m_bad++)

/**
 * Register a "good" message, i.e. one message processed in forward execution
 * @param auto_ckpt a pointer to the auto-checkpoint module struct of the current LP
 */
#define auto_ckpt_register_good(auto_ckpt) ((auto_ckpt)->m_good++)

/**
 * Get the currently computed optimal checkpointing interval
 * @param auto_ckpt a pointer to the auto-checkpoint module struct of the current LP
 * @return the optimal checkpointing interval for the current LP
 */
#define auto_ckpt_interval_get(auto_ckpt) ((auto_ckpt)->ckpt_interval)

/**
 * Register a new processed message and check if, for the given LP, a checkpoint is necessary
 * @param auto_ckpt a pointer to the auto-checkpoint module struct of the current LP
 * @return true if the LP needs a checkpoint, false otherwise
 */
#define auto_ckpt_is_needed(auto_ckpt, approx)                                                                         \
	__extension__({                                                                                                \
		_Bool r = ++(auto_ckpt)->ckpt_rem >=                                                                   \
		          ((approx) ? (auto_ckpt)->approx_ckpt_interval : (auto_ckpt)->ckpt_interval);                 \
		if(r)                                                                                                  \
			(auto_ckpt)->ckpt_rem = 0;                                                                     \
		r;                                                                                                     \
	})

extern void auto_ckpt_init(void);
extern void auto_ckpt_lp_init(struct auto_ckpt *auto_ckpt);
extern void auto_ckpt_on_gvt(void);
extern void auto_ckpt_lp_on_gvt(struct auto_ckpt *auto_ckpt, uint_fast32_t state_size, uint_fast32_t approx_state_size);
