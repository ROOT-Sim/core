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
	unsigned ckpt_interval;
	unsigned ckpt_rem;
};

#define auto_ckpt_register_bad(auto_ckpt) ((auto_ckpt)->m_bad++)
#define auto_ckpt_register_good(auto_ckpt) ((auto_ckpt)->m_good++)
#define auto_ckpt_interval_get(auto_ckpt) ((auto_ckpt)->ckpt_interval)

#define auto_ckpt_is_needed(auto_ckpt)                                                                                 \
	__extension__({                                                                                                \
		_Bool r = ++(auto_ckpt)->ckpt_rem >= (auto_ckpt)->ckpt_interval;                                       \
		if(r)                                                                                                  \
			(auto_ckpt)->ckpt_rem = 0;                                                                     \
		r;                                                                                                     \
	})

extern void auto_ckpt_init(void);
extern void auto_ckpt_lp_init(struct auto_ckpt *auto_ckpt);
extern void auto_ckpt_on_gvt(void);
extern void auto_ckpt_lp_on_gvt(struct auto_ckpt *auto_ckpt);
