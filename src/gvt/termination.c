#include <gvt/termination.h>

#include <gvt/gvt.h>
#include <lp/lp.h>

atomic_uint thr_to_end;

static __thread uint64_t lps_to_end;
static __thread simtime_t max_t;

void termination_global_init(void)
{
	atomic_store_explicit(&thr_to_end, n_threads, memory_order_relaxed);
}

void termination_lp_init(void)
{
	bool term = CanEnd(current_lp - lps, current_lp->lsm.state_s);
	lps_to_end += !term;
	current_lp->t_d = term * SIMTIME_MAX;
}

void termination_on_msg_process(void)
{
	lp_struct *this_lp = current_lp;
	if(this_lp->t_d) return;

	bool term = CanEnd(this_lp - lps, this_lp->lsm.state_s);
	simtime_t cur_t = current_msg->dest_t;
	max_t = term ? max(cur_t, max_t) : max_t;
	this_lp->t_d = term * cur_t;
	lps_to_end -= term;
}

void termination_on_gvt(void)
{
	if(
		unlikely(max_t < current_gvt &&
		(!lps_to_end || current_gvt == SIMTIME_MAX))
	){
		max_t = SIMTIME_MAX;
		atomic_fetch_sub_explicit(&thr_to_end, 1U, memory_order_relaxed);
	}
}

void termination_on_lp_rollback(void)
{
	lp_struct *this_lp = current_lp;
	simtime_t old_t = this_lp->t_d;
	bool keep = old_t < current_msg->dest_t || old_t == SIMTIME_MAX;
	this_lp->t_d = keep * old_t;
	lps_to_end += !keep;
}
