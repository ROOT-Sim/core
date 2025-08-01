/**
 * @file gvt/termination.c
 *
 * @brief Termination detection module
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <gvt/termination.h>

#include <datatypes/heap.h>

#include <assert.h>

#define term_queue_elem_is_before(a, b)                                                                                \
	((a).start != (a).end && ((b).start == (b).end || term_switch_ts(*(a).start) < term_switch_ts(*(b).start)))

union term_switch {
	simtime_t ts;
	uint64_t ts_bits;
};

struct term_queue_elem {
	union term_switch *start;
	union term_switch *end;
};

// Make sure the type punning nightmare works
_Static_assert(sizeof(simtime_t) == sizeof(uint64_t), "simtime_t and uint64_t must have the same size");
// Actually not strictly necessary to check for this
_Static_assert(_Alignof(simtime_t) == _Alignof(uint64_t), "simtime_t and uint64_t must have the same alignment");

static heap_declare(struct term_queue_elem) thread_termination_queue;
static array_declare(union term_switch) node_terminations;
static rid_t threads_to_terminate;
static _Thread_local array_declare(union term_switch) thread_terminations;
static _Thread_local array_count_t last_truncate_idx;
static _Thread_local lp_id_t lps_to_terminate;

static inline union term_switch term_switch_from(simtime_t ts, bool terminating)
{
	union term_switch sw = {.ts = ts};
	assert(!(sw.ts_bits & (uint64_t)1U << 63));
	sw.ts_bits |= (uint64_t)terminating << 63;
	return sw;
}

static inline simtime_t term_switch_ts(union term_switch sw)
{
	sw.ts_bits &= ~(uint64_t)1U;
	return sw.ts;
}

static inline bool term_switch_terminating(union term_switch sw)
{
	return (sw.ts_bits & (uint64_t)1U << 63) != 0;
}

void termination_on_change(simtime_t ts, bool is_terminating)
{
	array_count_t i = array_count(thread_terminations);
	while(i) {
		simtime_t this_ts = term_switch_ts(array_get_at(thread_terminations, --i));
		if(this_ts <= ts)
			break;
	}
	array_add_at(thread_terminations, i, term_switch_from(ts, is_terminating));
}

void termination_on_change_rollback(simtime_t ts, bool is_terminating)
{
	union term_switch rm = term_switch_from(ts, is_terminating);
	for(array_count_t i = array_count(thread_terminations); i;) {
		union term_switch term = array_get_at(thread_terminations, --i);
		if(rm.ts_bits == term.ts_bits) {
			array_remove_at(thread_terminations, i);
			break;
		}
	}
}

void termination_lp_to_thread_terminations(simtime_t gvt)
{
	array_truncate_first(thread_terminations, last_truncate_idx);

	array_count_t i = 0, j = 0;
	while(i < array_count(thread_terminations)) {
		union term_switch term = array_get_at(thread_terminations, i);
		simtime_t term_ts = term_switch_ts(term);
		if(term_ts >= gvt)
			break;

		bool terminating = term_switch_terminating(term);

		lps_to_terminate -= terminating;

		++i;
		// terminations with the same timestamp are considered concurrent; push only if the next ts is different
		if(unlikely(!lps_to_terminate && (i == array_count(thread_terminations) ||
						     term_switch_ts(array_get_at(thread_terminations, i)) != term_ts)))
			array_get_at(thread_terminations, j++) = term;

		lps_to_terminate += 1U - terminating;
	}

	last_truncate_idx = i;

	array_get_at(thread_termination_queue, rid) = (struct term_queue_elem){
	    .start = array_items(thread_terminations),
	    .end = array_items(thread_terminations) + j,
	};
}

void termination_thread_to_node_terminations(void)
{
	heap_count(thread_termination_queue) = global_config.n_threads;
	heap_heapify_from_array(thread_termination_queue, term_queue_elem_is_before);

	while(heap_count(thread_termination_queue)) {
		struct term_queue_elem tqe = heap_extract(thread_termination_queue, term_queue_elem_is_before);
		if(tqe.start == tqe.end)
			continue;

		union term_switch term = *tqe.start++;
		heap_insert_unsafe(thread_termination_queue, term_queue_elem_is_before, tqe);

		bool terminating = term_switch_terminating(term);

		threads_to_terminate -= terminating;

		struct term_queue_elem min_tqe = heap_min(thread_termination_queue);
		// terminations with the same timestamp are considered concurrent; push only if the next ts is different
		if(unlikely(!threads_to_terminate &&
			    (min_tqe.start == min_tqe.end || term_switch_ts(*min_tqe.start) != term_switch_ts(term))))
			array_push(node_terminations, term);

		threads_to_terminate += 1U - terminating;
	}
}

void mpi_allgatherv_u64(int send_count, uint64_t send_values[send_count], int all_count[n_nodes], uint64_t all_values[],
    const int displacements[n_nodes])
{
	MPI_Allgatherv(send_values, send_count, MPI_UINT64_T, all_values, all_count, displacements, MPI_UINT64_T,
	    MPI_COMM_WORLD);
}
/**
 * @brief Aggregate all the migrations from MPI ranks so that every rank has a consistent view of the migrations to do
 */
static void migration_disseminate(void)
{
	static int recv_counts[MAX_NODES], displs[MAX_NODES];

	int sendcount = array_count(locally_decided_migrations) * 2;
	mpi_allgather_int_single(sendcount, recv_counts);

	int total = 0;
	for(nid_t i = 0; i < n_nodes; ++i) {
		displs[i] = total;
		total += recv_counts[i];
	}

	array_reserve(migrations, total);
	mpi_allgatherv_u64(sendcount, (uint64_t *)array_items(locally_decided_migrations), recv_counts,
	    (uint64_t *)array_items(migrations), displs);
}

void termination_node_to_simulation_terminations(void)
{

}


enum termination_phase {
	termination_phase_local,
	termination_phase_
};

static _Thread_local enum termination_phase termination_phase;

void termination_on_gvt_phase(void)
{
	switch(termination_phase) {
		case termination_phase_local:

	}
}
