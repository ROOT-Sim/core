/**
* @file mm/minm/minm.c
*
* @brief A minimalistic memory allocator for simulation models
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
#include <mm/minm/minm.h>

#include <lp/lp.h>

#include <stdlib.h>
#include <errno.h>

#define M_CHKP_FREQ_EXP 4

void model_allocator_lp_init(void)
{
	struct mm_state *self = &current_lp->mm_state;
	array_init(self->chunks);
}

void model_allocator_lp_fini(void)
{
	struct mm_state *self = &current_lp->mm_state;
	array_fini(self->chunks);
}

void *__wrap_malloc(size_t req_size)
{
	return array_peek(current_lp->mm_state.chunks);
}

void __wrap_free(void *ptr)
{
	(void) ptr;
}

void *__wrap_realloc(void *ptr, size_t req_size)
{
	if(!req_size){
		__wrap_free(ptr);
		return NULL;
	}
	if(!ptr){
		return __wrap_malloc(req_size);
	}

	abort();
	return NULL;
}

void model_allocator_checkpoint_take(array_count_t ref_i)
{
	struct mm_state *self = &current_lp->mm_state;
	array_count_t cnt = array_count(self->chunks);

	if(cnt & ((1 << M_CHKP_FREQ_EXP) - 1))
		return;

	array_expand(self->chunks);
	memcpy(
		array_items(self->chunks) + cnt,
		array_items(self->chunks) + cnt -1,
		sizeof(*array_items(self->chunks))
	);
	array_count(self->chunks) = cnt + 1;
}

array_count_t model_allocator_checkpoint_restore(array_count_t ref_i)
{
	struct mm_state *self = &current_lp->mm_state;
	array_count(self->chunks) = ref_i >> M_CHKP_FREQ_EXP;
	return ref_i & ~((array_count_t)(1 << M_CHKP_FREQ_EXP) - 1);
}

array_count_t model_allocator_fossil_lp_collect(void)
{
	struct mm_state *self = &current_lp->mm_state;
	struct process_data *proc_p = &current_lp->p;
	array_count_t ref_i = array_count(self->chunks) << M_CHKP_FREQ_EXP;
	do {
		ref_i -= 1 << M_CHKP_FREQ_EXP;
	} while(process_cant_discard_ref_i(ref_i));
	array_truncate_first(self->chunks, ref_i >> M_CHKP_FREQ_EXP);
	return ref_i;
}
