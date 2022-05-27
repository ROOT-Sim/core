#include <lib/retractable/retractable.h>

#include <datatypes/retractable_heap.h>
#include <lp/lp.h>
#include <lp/process.h>
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

void retractable_lib_lp_init(void)
{
	struct lp_ctx *this_lp = current_lp;
	this_lp->retractable_pos = ARRAY_COUNT_MAX;
	this_lp->lib_ctx->retractable_t = SIMTIME_MAX;
}

void retractable_lib_fini(void)
{
	rheap_fini(r_queue);
}

static void retractable_reschedule(simtime_t t, struct lp_ctx *lp)
{
	array_count_t pos = lp->retractable_pos;
	if(pos == ARRAY_COUNT_MAX) {
		if(t != SIMTIME_MAX) {
			struct rq_elem rq = {.t = t, .lp = lp};
			rheap_insert(r_queue, rq_elem_is_before, rq_elem_update, rq);
		}
		return;
	}

	struct rq_elem rq = array_get_at(r_queue, pos);
	bool lowered = t > rq.t;
	array_get_at(r_queue, pos).t = t;

	if(lowered) {
		rheap_priority_lowered(r_queue, rq_elem_is_before, rq_elem_update, rq, pos);
		if (t == SIMTIME_MAX) {
			--array_count(r_queue);
			lp->retractable_pos = ARRAY_COUNT_MAX;
		}
	} else {
		rheap_priority_increased(r_queue, rq_elem_is_before, rq_elem_update, rq, pos);
	}
}

void retractable_rollback_handle(void)
{
	struct lp_ctx *this_lp = current_lp;
	retractable_reschedule(this_lp->lib_ctx->retractable_t, this_lp);
}

void ScheduleRetractableEvent(simtime_t timestamp)
{
	struct lp_ctx *this_lp = current_lp;
	this_lp->lib_ctx->retractable_t = timestamp;

	if(unlikely(process_is_silent()))
		return;

	retractable_reschedule(timestamp, this_lp);
}

struct lp_msg *retractable_extract(simtime_t normal_t)
{
	if(rheap_is_empty(r_queue) || rheap_min(r_queue).t > normal_t)
		return NULL;

	struct rq_elem rq = rheap_extract(r_queue, rq_elem_is_before, rq_elem_update);
	struct lp_msg *ret = msg_allocator_pack(rq.lp - lps, rq.t, LP_RETRACTABLE, NULL, 0);
	rq.lp->retractable_pos = ARRAY_COUNT_MAX;
	ret->raw_flags = 0;
	return ret;
}

simtime_t retractable_min_t(simtime_t normal_t)
{
	return rheap_is_empty(r_queue) ? normal_t : min(normal_t, rheap_min(r_queue).t);
}
