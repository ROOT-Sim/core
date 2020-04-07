#include <gvt/gvt.h>

#include <core/init.h>
#include <core/timer.h>
#include <datatypes/msg_queue.h>

#include <stdatomic.h>

enum thread_phase_t {
	tphase_idle,
	tphase_A,
	tphase_B,
	tphase_C,
	tphase_D
};

static atomic_uint counter_A = 0;
static atomic_uint counter_B = 0;

static __thread enum thread_phase_t thread_phase = tphase_idle;
static timer last_gvt;
static simtime_t *reducing_p;
static __thread simtime_t reducing;

__thread simtime_t current_gvt;

void gvt_global_init(void)
{
	reducing_p = mm_alloc(n_threads * sizeof(*reducing_p));
	last_gvt = timer_new();
}

void gvt_global_fini(void)
{
	mm_free(reducing_p);
}

static inline void reduce_thread_gvt(void)
{
	simtime_t candidate = msg_queue_time_peek();
	reducing = min(candidate, reducing);
}

static inline void reduce_node_gvt(void)
{
	unsigned i = n_threads - 1;
	simtime_t candidate = reducing_p[i];
	while(i--){
		candidate = min(reducing_p[i], candidate);
	}
	current_gvt = candidate;
}

bool gvt_msg_processed(void)
{
	switch (thread_phase) {
	case tphase_idle:
		if(rid){
			if (atomic_load_explicit(
				&counter_A, memory_order_relaxed) == 0){
				break;
			}
		} else {
			if (global_config.gvt_period > timer_value(last_gvt)){
				break;
			}
		}
		__attribute__ ((fallthrough));
	case tphase_A:
		reducing = SIMTIME_MAX;
		reduce_thread_gvt();
		thread_phase = tphase_B;
		atomic_fetch_add_explicit(&counter_A, 1U, memory_order_relaxed);
		break;
	case tphase_B:
		if (atomic_load_explicit(
			&counter_A, memory_order_relaxed) == n_threads){
			thread_phase = tphase_C;
			atomic_fetch_add_explicit(
				&counter_B, 1U, memory_order_relaxed);
		}
		break;
	case tphase_C:
		if (atomic_load_explicit(
			&counter_B, memory_order_relaxed) == n_threads){
			reduce_thread_gvt();
			reducing_p[rid] = reducing;
			thread_phase = tphase_D;
			atomic_fetch_sub_explicit(
				&counter_A, 1U, memory_order_relaxed);
		}
		break;
	case tphase_D:
		if (atomic_load_explicit(
			&counter_A, memory_order_relaxed) == 0){
			reduce_node_gvt();
			thread_phase = tphase_idle;
			atomic_fetch_sub_explicit(
				&counter_B, 1U, memory_order_relaxed);
			if(!rid){
				last_gvt = timer_new();
			}
			return true;
		}
		break;
	}
	return false;
}
