/**
 * @file mm/auto_ckpt.h
 *
 * @brief Autonomic checkpoint interval selection header
 *
 * The module which attempts to select the best checkpoint interval
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

/// Structure to keep data used for autonomic checkpointing selection
struct auto_ckpt {
	double inv_bad_p;
	unsigned m_bad;
	unsigned m_good;
	unsigned approx_ckpt_interval;
	unsigned ckpt_interval;
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
extern void auto_ckpt_lp_on_gvt(struct auto_ckpt *auto_ckpt);
