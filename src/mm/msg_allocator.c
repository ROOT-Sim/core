/**
* @file mm/msg_allocator.c
*
* @brief Memory management functions for messages
*
* Memory management functions for messages
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
#include <mm/msg_allocator.h>

#include <stddef.h>
#include <stdlib.h>

#include <core/core.h>
#include <datatypes/array.h>
#include <gvt/gvt.h>

static __thread dyn_array(lp_msg *) free_list = {0};
#ifndef NEUROME_SERIAL
static __thread dyn_array(lp_msg *) free_gvt_list = {0};
#endif

void msg_allocator_init(void)
{
	array_init(free_list);
#ifndef NEUROME_SERIAL
	array_init(free_gvt_list);
#endif
}

void msg_allocator_fini(void)
{
	log_log(LOG_TRACE, "[T %u] msg allocator fini", rid);
	while(!array_is_empty(free_list)){
		mm_free(array_pop(free_list));
	}
	array_fini(free_list);

#ifndef NEUROME_SERIAL
	while(!array_is_empty(free_gvt_list)){
		mm_free(array_pop(free_gvt_list));
	}
	array_fini(free_gvt_list);
#endif
}

lp_msg* msg_allocator_alloc(unsigned payload_size)
{
	lp_msg *ret;
	if(unlikely(payload_size > BASE_PAYLOAD_SIZE)){
		ret = mm_alloc(
			offsetof(lp_msg, extra_pl) +
			(payload_size - BASE_PAYLOAD_SIZE)
		);
		ret->pl_size = payload_size;
		return ret;
	}
	if(unlikely(array_is_empty(free_list))){
		ret = mm_alloc(sizeof(lp_msg));
		ret->pl_size = payload_size;
		return ret;
	}
	return array_pop(free_list);
}

void msg_allocator_free(lp_msg *msg)
{
	if(likely(msg->pl_size <= BASE_PAYLOAD_SIZE))
		array_push(free_list, msg);
	else
		mm_free(msg);
}

#ifdef NEUROME_MPI
void msg_allocator_free_at_gvt(lp_msg *msg)
{
	array_push(free_gvt_list, msg);
}

void msg_allocator_fossil_collect(simtime_t current_gvt)
{
	array_count_t j = 0;
	for(array_count_t i = 0; i < array_count(free_gvt_list); ++i){
		lp_msg *msg = array_get_at(free_gvt_list, i);
		// xxx this could need to check the actual sending time instead
		// of the destination time in order to avoid a pseudo memory
		// leak for LPs which send messages distant in logical time.
		if(current_gvt > msg->dest_t){
			msg_allocator_free(msg);
		} else {
			array_get_at(free_gvt_list, j) = msg;
			j++;
		}
	}
	array_count(free_gvt_list) = j;
}
#endif

extern lp_msg* msg_allocator_pack(lp_id_t receiver, simtime_t timestamp,
	unsigned event_type, const void *payload, unsigned payload_size);
