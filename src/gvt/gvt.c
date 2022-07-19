/**
 * @file gvt/gvt.c
 *
 * @brief Global Virtual Time
 *
 * For a detailed explanantion of the algorithm, see:
 *
 * T. Tocci, A. Pellegrini, F. Quaglia, J. Casanovas-García, and T. Suzumura
 * “ORCHESTRA: An Asynchronous Wait-Free Distributed GVT Algorithm”
 * in Proceedings of the 21st IEEE/ACM International Symposium on Distributed
 * Simulation and Real Time Applications, 2017, pp. 51–58
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <gvt/gvt.h>

#include <arch/timer.h>
#include <core/sync.h>
#include <datatypes/msg_queue.h>
#include <distributed/mpi.h>

#include <memory.h>
#include <stdatomic.h>

/// A thread phase during the gvt algorithm computation
enum thread_phase { thread_phase_idle = 0, thread_phase_A, thread_phase_B, thread_phase_C, thread_phase_D };

/// Node phases of the non-blocking GVT algorithm
enum node_phase {
	node_phase_redux_first = 0,
	node_sent_reduce,
	node_sent_reduce_wait,
	node_sent_wait,
	node_phase_redux_second,
	node_min_reduce,
	node_min_reduce_wait,
	node_min_wait,
	node_done
};

static __thread enum thread_phase thread_phase = thread_phase_idle;

static timer_uint gvt_timer;
static simtime_t reducing_p[MAX_THREADS];
static __thread simtime_t current_gvt;
static _Atomic rid_t c_a = 0;
static _Atomic rid_t c_b = 0;

static _Atomic nid_t gvt_nodes;

__thread _Bool gvt_phase;
__thread uint32_t remote_msg_seq[2][MAX_NODES];
__thread uint32_t remote_msg_received[2];

/**
 * @brief Initializes the gvt module in the node
 */
void gvt_global_init(void)
{
	gvt_timer = timer_new();
}

/**
 * @brief Handles a MSG_CTRL_GVT_START control message
 *
 * Called by the MPI layer in response to a MSG_CTRL_GVT_START control message,
 * but also internally to start a new reduction
 */
void gvt_start_processing(void)
{
	current_gvt = SIMTIME_MAX;
	thread_phase = thread_phase_A;
}

/**
 * @brief Handles a MSG_CTRL_GVT_DONE control message
 *
 * Called by the MPI layer in response to a MSG_CTRL_GVT_DONE control message
 */
void gvt_on_done_ctrl_msg(void)
{
	atomic_fetch_sub_explicit(&gvt_nodes, 1U, memory_order_relaxed);
}

/**
 * @brief Informs the GVT subsystem that a new message is being processed
 * @param msg_t the timestamp of the message being processed
 *
 * Called by the process layer when processing a new message; used in the actual GVT calculation
 */
void gvt_on_msg_extraction(simtime_t msg_t)
{
	if(unlikely(current_gvt > msg_t))
		current_gvt = msg_t;
}

static inline simtime_t gvt_node_reduce(void)
{
	unsigned i = global_config.n_threads - 1;
	simtime_t candidate = reducing_p[i];
	while(i--)
		candidate = min(reducing_p[i], candidate);

	return candidate;
}

static bool gvt_thread_phase_run(void)
{
	switch(thread_phase) {
		case thread_phase_A:
			if(atomic_load_explicit(&c_a, memory_order_relaxed))
				break;
			current_gvt = min(current_gvt, msg_queue_time_peek());
			thread_phase = thread_phase_B;
			atomic_fetch_add_explicit(&c_b, 1U, memory_order_relaxed);
			break;
		case thread_phase_B:
			if(atomic_load_explicit(&c_b, memory_order_relaxed) != global_config.n_threads)
				break;
			thread_phase = thread_phase_C;
			atomic_fetch_add_explicit(&c_a, 1U, memory_order_relaxed);
			break;
		case thread_phase_C:
			if(atomic_load_explicit(&c_a, memory_order_relaxed) != global_config.n_threads)
				break;
			reducing_p[rid] = min(current_gvt, msg_queue_time_peek());
			thread_phase = thread_phase_D;
			atomic_fetch_sub_explicit(&c_b, 1U, memory_order_release);
			break;
		case thread_phase_D:
			if(atomic_load_explicit(&c_b, memory_order_acquire))
				break;
			thread_phase = thread_phase_idle;
			atomic_fetch_sub_explicit(&c_a, 1U, memory_order_relaxed);
			return true;
		default:
			__builtin_unreachable();
	}
	return false;
}

/**
 * @fn gvt_phase_run(void)
 * @brief Executes a step of the GVT algorithm
 * @return the latest GVT value or 0.0 if the algorithm must still complete
 *
 * This function must be called several times by all the processing threads
 * hosted in the nodes involved in the simulation before completing and
 * returning a proper GVT value.
 */


/**
 * @fn gvt_msg_drain(void)
 * @brief Cleanup the distributed state of the GVT algorithm
 *
 * This function flushes the remaining remote messages, moreover it makes sure
 * that all the nodes complete the potentially ongoing GVT algorithm so that
 * MPI collectives are inactive.
 */


static bool gvt_node_phase_run(void)
{
	static __thread enum node_phase node_phase = node_phase_redux_first;
	static __thread uint32_t last_seq[2][MAX_NODES];
	static _Atomic(uint32_t) total_sent[MAX_NODES];
	static _Atomic(int32_t) total_msg_received;
	static uint32_t remote_msg_to_receive;
	static _Atomic(rid_t) c_c;
	static _Atomic(rid_t) c_d;

	switch(node_phase) {
		case node_phase_redux_first:
		case node_phase_redux_second:
			if(!gvt_thread_phase_run())
				break;

			gvt_phase = gvt_phase ^ (!node_phase);
			thread_phase = thread_phase_A;
			++node_phase;
			break;
		case node_sent_reduce:
			if(atomic_load_explicit(&c_a, memory_order_relaxed))
				break;

			for(nid_t i = n_nodes - 1; i >= 0; --i)
				atomic_fetch_add_explicit(&total_sent[i],
				    remote_msg_seq[!gvt_phase][i] - last_seq[!gvt_phase][i], memory_order_relaxed);
			memcpy(last_seq[!gvt_phase], remote_msg_seq[!gvt_phase], sizeof(uint32_t) * n_nodes);

			atomic_fetch_add_explicit(&total_msg_received, 1U, memory_order_relaxed);
			// synchronizes total_sent and sent values zeroing
			if(atomic_fetch_add_explicit(&c_c, 1U, memory_order_acq_rel) != global_config.n_threads - 1) {
				node_phase = node_sent_wait;
				break;
			}
			mpi_reduce_sum_scatter((uint32_t *)total_sent, &remote_msg_to_receive);
			node_phase = node_sent_reduce_wait;
			break;
		case node_sent_reduce_wait:
			if(!mpi_reduce_sum_scatter_done())
				break;
			atomic_fetch_sub_explicit(&total_msg_received, remote_msg_to_receive + global_config.n_threads,
			    memory_order_relaxed);
			node_phase = node_sent_wait;
			break;
		case node_sent_wait:
			{
				int32_t r = atomic_fetch_add_explicit(&total_msg_received,
				    remote_msg_received[!gvt_phase], memory_order_relaxed);
				remote_msg_received[!gvt_phase] = 0;
				if(r)
					break;
				uint32_t q = n_nodes / global_config.n_threads + 1;
				memset(total_sent + rid * q, 0, q * sizeof(*total_sent));
				node_phase = node_phase_redux_second;
				break;
			}
		case node_min_reduce:
			if(atomic_fetch_add_explicit(&c_d, 1U, memory_order_relaxed)) {
				node_phase = node_min_wait;
				break;
			}
			*reducing_p = gvt_node_reduce();
			mpi_reduce_min(reducing_p);
			node_phase = node_min_reduce_wait;
			break;
		case node_min_reduce_wait:
			if(atomic_load_explicit(&c_d, memory_order_relaxed) != global_config.n_threads ||
			    !mpi_reduce_min_done())
				break;
			atomic_fetch_sub_explicit(&c_c, global_config.n_threads, memory_order_release);
			node_phase = node_done;
			return true;
		case node_min_wait:
			if(atomic_load_explicit(&c_c, memory_order_acquire))
				break;
			node_phase = node_done;
			return true;
		case node_done:
			node_phase = node_phase_redux_first;
			thread_phase = thread_phase_idle;
			if(atomic_fetch_sub_explicit(&c_d, 1U, memory_order_relaxed) == 1)
				mpi_control_msg_send_to(MSG_CTRL_GVT_DONE, 0);
			break;
		default:
			__builtin_unreachable();
	}
	return false;
}

simtime_t gvt_phase_run(void)
{
	if(unlikely(thread_phase))
		return gvt_node_phase_run() ? *reducing_p : 0.0;

	if(unlikely(atomic_load_explicit(&c_b, memory_order_relaxed)))
		gvt_start_processing();

	if(unlikely(!rid && !nid)) {
		timer_uint t = timer_new();
		if(unlikely(global_config.gvt_period < t - gvt_timer &&
			    !atomic_load_explicit(&gvt_nodes, memory_order_relaxed))) {
			gvt_timer = t;
			atomic_fetch_add_explicit(&gvt_nodes, n_nodes, memory_order_relaxed);
			mpi_control_msg_broadcast(MSG_CTRL_GVT_START);
		}
	}

	return 0.0;
}

void gvt_msg_drain(void)
{
	while(thread_phase != thread_phase_idle) // flush partial gvt algorithm
		gvt_phase_run();

	if(sync_thread_barrier())
		mpi_node_barrier();
	sync_thread_barrier();

	for(int i = 0; i < 2; ++i) { // flush both gvt phases
		gvt_timer = 0;       // this satisfies the timer condition
		while(!gvt_phase_run())
			mpi_remote_msg_drain();
	}
}

/**
 * Registers an outgoing remote message in the GVT subsystem
 * @param msg the remote message to register
 * @param dest_nid the destination node id of the message
 */
extern void gvt_remote_msg_send(struct lp_msg *msg, nid_t dest_nid);

/**
 * Registers an outgoing remote anti-message in the GVT subsystem
 * @param msg the remote anti-message to register
 * @param dest_nid the destination node id of the anti-message
 */
extern void gvt_remote_anti_msg_send(struct lp_msg *msg, nid_t dest_nid);

/**
 * Registers an incoming remote message in the GVT subsystem
 * @param msg the remote message to register
 */
extern void gvt_remote_msg_receive(struct lp_msg *msg);

/**
 * Registers an incoming remote anti-message in the GVT subsystem
 * @param msg the remote anti-message to register
 */
extern void gvt_remote_anti_msg_receive(struct lp_msg *msg);
