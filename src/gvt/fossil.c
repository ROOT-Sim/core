/**
* @file gvt/fossil.c
*
* @brief Housekeeping operations
*
* In this module all the housekeeping operations related to GVT computation phase
* are present.
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <gvt/fossil.h>

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
#ifdef NEUROME_MPI
	remote_msg_map_fossil_collect(current_gvt);
	msg_allocator_fossil_collect(current_gvt);
#endif

	uint64_t i, lps_cnt;
	lps_iter_init(i, lps_cnt);

	while(lps_cnt--){
		current_lp = &lps[i];
		fossil_lp_collect(current_gvt);
		i++;
	}
}
