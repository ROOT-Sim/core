/**
* @file gvt/termination.c
*
* @brief Termination detection module
*
* This module implements the termination detection checks.
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
#include <gvt/termination.h>

#include <core/init.h>
#include <distributed/mpi.h>
#include <gvt/gvt.h>
#include <lp/lp.h>

atomic_uint thr_to_end;
atomic_uint nodes_to_end;

static __thread uint64_t lps_to_end;
static __thread simtime_t max_t;

void termination_global_init(void)
{
	atomic_store_explicit(&thr_to_end, n_threads, memory_order_relaxed);
	atomic_store_explicit(&nodes_to_end, n_nodes, memory_order_relaxed);
}

void termination_lp_init(void)
{
	bool term = CanEnd(current_lp - lps, current_lp->lsm_p->state_s);
	lps_to_end += !term;
	current_lp->t_d = term * SIMTIME_MAX;
}

void termination_on_msg_process(simtime_t msg_time)
{
	lp_struct *this_lp = current_lp;
	if(this_lp->t_d) return;

	bool term = CanEnd(this_lp - lps, this_lp->lsm_p->state_s);
	max_t = term ? max(msg_time, max_t) : max_t;
	this_lp->t_d = term * msg_time;
	lps_to_end -= term;
}

void termination_on_ctrl_msg(void)
{
	atomic_fetch_sub_explicit(&nodes_to_end, 1U, memory_order_relaxed);
}

void termination_on_gvt(simtime_t current_gvt)
{
	if (unlikely((max_t < current_gvt && !lps_to_end) ||
			current_gvt >= global_config.termination_time)) {
		max_t = SIMTIME_MAX;
		unsigned t = atomic_fetch_sub_explicit(&thr_to_end, 1U,
			memory_order_relaxed) - 1;
		if(!t){
			atomic_fetch_sub_explicit(&nodes_to_end, 1U,
				memory_order_relaxed);
#ifdef NEUROME_MPI
			mpi_control_msg_broadcast(MSG_CTRL_TERMINATION);
#endif
		}
	}
}

void termination_on_lp_rollback(simtime_t msg_time)
{
	lp_struct *this_lp = current_lp;
	simtime_t old_t = this_lp->t_d;
	bool keep = old_t < msg_time || old_t == SIMTIME_MAX;
	this_lp->t_d = keep * old_t;
	lps_to_end += !keep;
}
