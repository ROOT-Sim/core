#include <lib/retractable/retractable.h>

#include <datatypes/retractable_heap.h>
#include <mm/msg_allocator.h>

#define rq_elem_is_before(a, b) ((a).t < (b).t)
#define rq_elem_update(rq, i) ((rq).lp->retractable_pos = (i))

struct rq_elem {
	simtime_t t;
	struct lp_ctx *lp;
};

static __thread rheap_declare(struct rq_elem) r_queue;

void retractable_lib_init(void)
{
	rheap_init(r_queue);
}

void retractable_lib_lp_init(struct lp_ctx *this_lp)
{
	this_lp->retractable_ctx = rs_malloc(sizeof(*this_lp->retractable_ctx));
	*this_lp->retractable_ctx = SIMTIME_MAX;
	struct rq_elem rq = {.t = SIMTIME_MAX, .lp = this_lp};
	rheap_insert(r_queue, rq_elem_is_before, rq_elem_update, rq);
}

void retractable_lib_fini(void)
{
	rheap_fini(r_queue);
}

void retractable_reschedule(const struct lp_ctx *lp)
{
	array_count_t pos = lp->retractable_pos;
	struct rq_elem rq = array_get_at(r_queue, pos);
	simtime_t t = *lp->retractable_ctx;
	if(t == rq.t)
		return;

	bool lowered = t > rq.t;
	array_get_at(r_queue, pos).t = t;
	rq.t = t;

	if(lowered) {
		rheap_priority_lowered(r_queue, rq_elem_is_before, rq_elem_update, rq, pos);
	} else {
		rheap_priority_increased(r_queue, rq_elem_is_before, rq_elem_update, rq, pos);
	}
}

void ScheduleRetractableEvent(simtime_t timestamp)
{
	*current_lp->retractable_ctx = timestamp;
}

void retractable_post_silent(const struct lp_ctx *lp, simtime_t now)
{
	if(*lp->retractable_ctx <= now) // leq because retractable msgs have the precedence
		*lp->retractable_ctx = SIMTIME_MAX;
}

struct lp_msg *retractable_extract(void)
{
	struct rq_elem rq = rheap_min(r_queue);
	*rq.lp->retractable_ctx = SIMTIME_MAX;
	struct lp_msg *ret = msg_allocator_pack(rq.lp - lps, rq.t, LP_RETRACTABLE, NULL, 0);
	ret->raw_flags = 0;
	return ret;
}

bool retractable_is_before(simtime_t normal_t)
{
	return rheap_min(r_queue).t <= normal_t && likely(rheap_min(r_queue).t != SIMTIME_MAX);
}
