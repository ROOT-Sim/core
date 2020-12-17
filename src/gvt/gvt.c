/**
* @file gvt/gvt.c
*
* @brief Global Virtual Time
*
* This module implements the GVT reduction. The current implementation
* is non blocking for observable simulation plaftorms.
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
#include <gvt/gvt.h>

#include <arch/timer.h>
#include <core/init.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>
#include <log/stats.h>

#include <stdatomic.h>
#include <memory.h>

enum thread_phase_t {
	tphase_rdy = 0,
	tphase_A,
	tphase_B,
#ifdef ROOTSIM_MPI
	tphase_B_reduce,
	tphase_C,
	tphase_C_reduce,
	tphase_B_rdone,
	tphase_C_rdone,
	tphase_B_wait_msgs,
#endif
	tphase_wait
};

static __thread enum thread_phase_t thread_phase = tphase_rdy;

static timer_uint last_gvt;
static simtime_t reducing_p[1 << MAX_THREADS_BITS];
static __thread simtime_t current_gvt;

#ifdef ROOTSIM_MPI

static atomic_uint sent_tot[1 << MAX_NODES_BITS];
static unsigned remote_msg_to_receive;
static _Atomic(nid_t) missing_nodes;

__thread bool gvt_phase_green;
__thread unsigned remote_msg_sent[1 << MAX_NODES_BITS] = {0};
atomic_int remote_msg_received[2];

#endif

void gvt_global_init(void)
{
	last_gvt = timer_new();
#ifdef ROOTSIM_MPI
	missing_nodes = 0;
#endif
}

static inline simtime_t gvt_node_reduce(void)
{
	unsigned i = n_threads - 1;
	simtime_t candidate = reducing_p[i];
	while(i--){
		candidate = min(reducing_p[i], candidate);
	}
	return candidate;
}

#ifndef ROOTSIM_MPI

simtime_t gvt_phase_run(void)
{
	static atomic_uint c_a = 0;
	static atomic_uint c_b = 0;

	if(likely(thread_phase == tphase_rdy)) {
		if (rid) {
			if (likely(!atomic_load_explicit(&c_a,
				memory_order_relaxed)))
				return false;
		} else {
			if (likely(global_config.gvt_period >
				timer_value(last_gvt) || atomic_load_explicit(
				&c_b, memory_order_relaxed)))
				return false;
		}
		stats_time_start(STATS_GVT);
		current_gvt = SIMTIME_MAX;
		thread_phase = tphase_A;
		atomic_fetch_add_explicit(&c_a, 1U, memory_order_relaxed);
		return 0;
	}

	switch (thread_phase) {
	default:
	case tphase_rdy:
		__builtin_unreachable();
		/* fallthrough */
	case tphase_A:
		if (atomic_load_explicit(&c_a, memory_order_relaxed)
			== n_threads) {
			simtime_t this_t = msg_queue_time_peek();
			reducing_p[rid] = min(current_gvt, this_t);
			thread_phase = tphase_B;
			atomic_fetch_add_explicit(&c_b, 1U,
				memory_order_release);
		}
		break;
	case tphase_B:
		if (atomic_load_explicit(&c_b, memory_order_acquire)
			== n_threads) {
			thread_phase = tphase_wait;
			atomic_fetch_sub_explicit(&c_a, 1U,
				memory_order_relaxed);
			if(!rid){
				last_gvt = timer_new();
			}
			return gvt_node_reduce();
		}
		break;
	case tphase_wait:
		if (!atomic_load_explicit(&c_a, memory_order_relaxed)) {
			atomic_fetch_sub_explicit(&c_b, 1U,
				memory_order_relaxed);
			thread_phase = tphase_rdy;
			stats_time_take(STATS_GVT);
		}
		break;
	}

	return 0;
}

#else

static atomic_uint c_a = 0;

void gvt_on_start_ctrl_msg(void)
{
	current_gvt = SIMTIME_MAX;
	thread_phase = tphase_A;
	atomic_fetch_add_explicit(&c_a, 1U, memory_order_relaxed);
}

void gvt_on_done_ctrl_msg(void)
{
	atomic_fetch_sub_explicit(&missing_nodes, 1U, memory_order_relaxed);
}

simtime_t gvt_phase_run(void)
{
	static atomic_uint c_b = 0;
	static __thread bool red_round = false;

	if(likely(thread_phase == tphase_rdy)) {
		if (nid || rid) {
			if (likely(!atomic_load_explicit(&c_a,
				memory_order_relaxed)))
				return false;
		} else {
			if (likely(global_config.gvt_period >
				timer_value(last_gvt) || atomic_load_explicit(
				&missing_nodes, memory_order_relaxed)))
				return false;

			atomic_store_explicit(&missing_nodes, n_nodes,
				memory_order_relaxed);
			mpi_control_msg_broadcast(MSG_CTRL_GVT_START);
		}
		stats_time_start(STATS_GVT);
		current_gvt = SIMTIME_MAX;
		thread_phase = tphase_A;
		atomic_fetch_add_explicit(&c_a, 1U, memory_order_relaxed);
		return 0;
	}

	switch (thread_phase) {
	default:
	case tphase_rdy:
		__builtin_unreachable();
		/* fallthrough */
	case tphase_A:
		if (atomic_load_explicit(&c_a, memory_order_relaxed)
			== n_threads) {
			simtime_t this_t = msg_queue_time_peek();
			// this leverages the enum layout
			thread_phase = tphase_B + (2 * red_round) + !rid;

			red_round = !red_round;
			if(red_round){
				for(nid_t i = 0; i < n_nodes; ++i)
					atomic_fetch_add_explicit(&sent_tot[i],
						remote_msg_sent[i],
						memory_order_relaxed);
				// release renders visible the sum aggregation
				atomic_fetch_add_explicit(&c_b, 1U,
					memory_order_release);

				memset(remote_msg_sent, 0,
					sizeof(unsigned) * n_nodes);
				gvt_phase_green = !gvt_phase_green;
				current_gvt = min(current_gvt, this_t);
			} else {
				reducing_p[rid] = min(current_gvt, this_t);
				// release renders visible the thread local gvt
				atomic_fetch_add_explicit(&c_b, 1U,
					memory_order_release);
			}
		}
		break;
	case tphase_B:
		// acquire is needed to sync with the remote_msg_received sum
		// and the c_b counter
		if (!atomic_load_explicit(&c_a, memory_order_acquire))
			thread_phase = tphase_B_wait_msgs;
		break;
	case tphase_B_reduce:
		// acquire is needed to sync with all threads aggregated values
		if (atomic_load_explicit(&c_b, memory_order_acquire) ==
			n_threads) {
			mpi_reduce_sum_scatter((unsigned *)sent_tot,
				&remote_msg_to_receive);
			thread_phase = tphase_B_rdone;
		}
		break;
	case tphase_B_rdone:
		if (mpi_reduce_sum_scatter_done()) {
			atomic_fetch_sub_explicit(remote_msg_received +
				!gvt_phase_green, remote_msg_to_receive,
				memory_order_relaxed);
			// release renders visible the remote_msg_received sum
			// and the c_b counter
			atomic_store_explicit(&c_a, 0, memory_order_release);
			memset(sent_tot, 0, sizeof(atomic_uint) * n_nodes);
			thread_phase = tphase_B_wait_msgs;
		}
		break;
	case tphase_C:
		// no sync needed
		if (!atomic_load_explicit(&c_a, memory_order_relaxed)) {
			atomic_fetch_sub_explicit(&c_b, 1U,
				memory_order_relaxed);
			thread_phase = tphase_wait;
			return *reducing_p;
		}
		break;
	case tphase_C_reduce:
		// acquire is needed to sync with all threads gvt local values
		if (atomic_load_explicit(&c_b, memory_order_acquire) ==
			n_threads) {
			*reducing_p = gvt_node_reduce();
			mpi_reduce_min(reducing_p);
			thread_phase = tphase_C_rdone;
		}
		break;
	case tphase_C_rdone:
		if (mpi_reduce_min_done()) {
			atomic_fetch_sub_explicit(&c_b, 1U,
				memory_order_relaxed);
			// no sync needed
			atomic_store_explicit(&c_a, 0, memory_order_relaxed);
			last_gvt = timer_new();
			thread_phase = tphase_wait;
			return *reducing_p;
		}
		break;
	case tphase_B_wait_msgs:
		// don't need to sync: we already synced on c_f in tphase_D
		if(!atomic_load_explicit(remote_msg_received + !gvt_phase_green,
			memory_order_relaxed)) {
			thread_phase = tphase_wait;
			atomic_fetch_sub_explicit(&c_b, 1U,
				memory_order_relaxed);
		}
		break;
	case tphase_wait:
		if (!atomic_load_explicit(&c_b, memory_order_relaxed)) {
		// this restarts the gvt phases for the second round of node
		// local gvt reduction or, if we already did that, it simply
		// ends the gvt reduction
			thread_phase = red_round;
			if(red_round) {
				atomic_fetch_add_explicit(&c_a, 1U,
					memory_order_relaxed);
			} else {
				if(!rid)
					mpi_control_msg_send_to(
						MSG_CTRL_GVT_DONE, 0);
				stats_time_take(STATS_GVT);
			}
		}
		break;
	}

	return 0;
}

#endif

void gvt_on_msg_process(simtime_t msg_t)
{
	if(unlikely(thread_phase && current_gvt > msg_t)){
		current_gvt = msg_t;
	}
}
