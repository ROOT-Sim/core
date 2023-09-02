/**
 * @file gvt/fossil.c
 *
 * @brief Housekeeping operations
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <gvt/fossil.h>

#include <mm/msg_allocator.h>

__thread unsigned fossil_epoch_current;
/// The value of the last GVT, kept here for easier fossil collection operations
static __thread simtime_t fossil_gvt_current;

/**
 * @brief Perform fossil collection operations at a given GVT
 * @param this_gvt The value of the freshly computed GVT
 */
void fossil_on_gvt(simtime_t this_gvt)
{
	fossil_epoch_current += 1;
	fossil_gvt_current = this_gvt;
}

/**
 * @brief Perform fossil collection for the data structures of a certain LP
 * @param lp The LP on which to perform fossil collection
 */
void fossil_lp_collect(struct lp_ctx *lp)
{
	struct process_ctx *proc_p = &lp->p;

	array_count_t past_i = array_count(proc_p->p_msgs);
	if(past_i == 0)
		return;

	simtime_t gvt = fossil_gvt_current;
	for(const struct lp_msg *msg = array_get_at(proc_p->p_msgs, --past_i); msg->dest_t >= gvt;) {
		do {
			if(!past_i)
				return;
			msg = array_get_at(proc_p->p_msgs, --past_i);
		} while(proc_is_sent(msg));
	}

	past_i = model_allocator_fossil_lp_collect(&lp->mm_state, past_i + 1);

	array_count_t k = past_i;
	while(k--) {
		struct lp_msg *msg = array_get_at(proc_p->p_msgs, k);
		if(!proc_is_sent_local(msg))
			msg_allocator_free(proc_untagged(msg));
	}
	array_truncate_first(proc_p->p_msgs, past_i);

	lp->fossil_epoch = fossil_epoch_current;
}
