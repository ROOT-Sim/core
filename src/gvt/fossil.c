/**
 * @file gvt/fossil.c
 *
 * @brief Housekeeping operations
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <gvt/fossil.h>

#include <gvt/gvt.h>
#include <lp/lp.h>
#include <lp/process.h>
#include <mm/model_allocator.h>
#include <mm/msg_allocator.h>

#include <memory.h>

void fossil_lp_on_gvt(struct lp_ctx *lp, simtime_t current_gvt)
{
	struct process_data *proc_p = &lp->p;

	array_count_t past_i = array_count(proc_p->p_msgs);
	if (past_i == 0)
		return;

	const struct lp_msg *msg = array_get_at(proc_p->p_msgs, --past_i);
	do {
		if (msg->dest_t < current_gvt)
			break;
		while (1) {
			if (!past_i)
				return;
			msg = array_get_at(proc_p->p_msgs, --past_i);
			if (is_msg_past(msg))
				break;
		}
	} while(1);

	past_i = model_allocator_fossil_lp_collect(&lp->mm_state, past_i + 1);

	array_count_t k = past_i;
	while (k--) {
		struct lp_msg *msg = array_get_at(proc_p->p_msgs, k);
		if (!is_msg_local_sent(msg))
			msg_allocator_free(unmark_msg(msg));
	}
	array_truncate_first(proc_p->p_msgs, past_i);
}
