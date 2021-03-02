/**
 * @file gvt/fossil.c
 *
 * @brief Housekeeping operations
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <gvt/fossil.h>

#include <datatypes/remote_msg_map.h>
#include <gvt/gvt.h>
#include <lp/lp.h>
#include <lp/process.h>
#include <mm/model_allocator.h>
#include <mm/msg_allocator.h>

#include <memory.h>

#define cant_discard_ref_i(ref_i) \
	array_get_at(proc_p->past_msgs, ref_i)->dest_t >= current_gvt

static inline array_count_t first_discardable_ref(simtime_t current_gvt,
	struct process_data *proc_p)
{
	array_count_t j = array_count(proc_p->past_msgs) - 1;
	while(cant_discard_ref_i(j)){
		--j;
	}
	return j;
}

static inline void fossil_lp_collect(simtime_t current_gvt)
{
	struct process_data *proc_p = &current_lp->p;
	array_count_t past_i = first_discardable_ref(current_gvt, proc_p);
	past_i = model_allocator_fossil_lp_collect(past_i);
	array_count_t sent_i = array_count(proc_p->sent_msgs);
	array_count_t j = array_count(proc_p->past_msgs);
	do{
		--sent_i;
		j -= (array_get_at(proc_p->sent_msgs, sent_i) == NULL);
	} while(j > past_i);
	array_truncate_first(proc_p->sent_msgs, sent_i);

	while(j--){
		msg_allocator_free(array_get_at(proc_p->past_msgs, j));
	}
	array_truncate_first(proc_p->past_msgs, past_i);
}

void fossil_collect(simtime_t current_gvt)
{
#ifdef ROOTSIM_MPI
	remote_msg_map_fossil_collect(current_gvt);
	msg_allocator_fossil_collect(current_gvt);
#endif

	for (uint64_t i = lp_id_first; i < lp_id_end; ++i) {
		current_lp = &lps[i];
		fossil_lp_collect(current_gvt);
	}
}
