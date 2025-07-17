/**
 * @file gvt/termination.c
 *
 * @brief Termination detection module
 *
 * SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <gvt/termination.h>

#include <assert.h>

#define term_queue_elem_is_before(a, b) \
__extension__({                         \
                                        \
})

union term_switch {
	simtime_t ts;
	uint64_t ts_bits;
};

typedef array_declare(union term_switch) term_switch_array;

struct term_queue_elem {
	term_switch_array *array;
	array_count_t idx;
};

// Make sure the type punning nightmare works
_Static_assert(sizeof(simtime_t) == sizeof(uint64_t), "simtime_t and uint64_t must have the same size");
// Actually not strictly necessary to check for this
_Static_assert(_Alignof(simtime_t) == _Alignof(uint64_t), "simtime_t and uint64_t must have the same alignment");

static term_switch_array node_terminations;
static rid_t threads_to_terminate;
static _Thread_local term_switch_array thread_terminations;
static _Thread_local term_switch_array lp_terminations;
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
	array_count_t i = array_count(lp_terminations);
	while(i) {
		simtime_t this_ts = term_switch_ts(array_get_at(lp_terminations, --i));
		if(this_ts <= ts)
			break;
	}
	array_add_at(lp_terminations, i, term_switch_from(ts, is_terminating));
}

void termination_on_change_rollback(simtime_t ts, bool is_terminating)
{
	union term_switch rm = term_switch_from(ts, is_terminating);
	for(array_count_t i = array_count(lp_terminations); i;) {
		union term_switch term = array_get_at(lp_terminations, --i);
		if(rm.ts_bits == term.ts_bits) {
			array_remove_at(lp_terminations, i);
			break;
		}
	}
}

void termination_lp_to_thread_terminations(simtime_t gvt)
{
	array_count(thread_terminations) = 0;
	array_count_t i = 0;
	while(i < array_count(lp_terminations)) {
		union term_switch term = array_get_at(lp_terminations, i);
		simtime_t term_ts = term_switch_ts(term);
		if(term_ts >= gvt)
			break;

		bool terminating = term_switch_terminating(term);

		lps_to_terminate -= terminating;

		++i;
		// terminations with the same timestamp are considered concurrent; push only if the next ts is different
		if(unlikely(!lps_to_terminate && (i == array_count(lp_terminations) ||
						    term_switch_ts(array_get_at(lp_terminations, i)) != term_ts)))
			array_push(thread_terminations, term);

		lps_to_terminate += 1U - terminating;
	}
	array_truncate_first(lp_terminations, i);
}

void termination_thread_to_node_terminations(simtime_t gvt)
{
	array_count(node_terminations) = 0;
	array_count_t i = 0;
	while(i < array_count(lp_terminations)) {
		union term_switch term = array_get_at(lp_terminations, i);
		simtime_t term_ts = term_switch_ts(term);
		if(term_ts >= gvt)
			break;

		bool terminating = term_switch_terminating(term);

		lps_to_terminate -= terminating;

		++i;
		// terminations with the same timestamp are considered concurrent; push only if the next ts is different
		if(unlikely(!lps_to_terminate && (i == array_count(lp_terminations) ||
						     term_switch_ts(array_get_at(lp_terminations, i)) != term_ts)))
			array_push(thread_terminations, term);

		lps_to_terminate += 1U - terminating;
	}
	array_truncate_first(lp_terminations, i);
}
