#include <lp/retractable.h>

#include <datatypes/retractable_heap.h>
#include <mm/msg_allocator.h>

#include <assert.h>

#define rq_elem_is_before(a, b) ((a).t < (b).t)
#define rq_elem_update(rq, i) ((rq).lp->retractable_entry_pos = (i))

struct rq_elem {
	simtime_t t;
	struct lp_ctx *lp;
};

static _Thread_local retractable_heap_declare(struct rq_elem) retractable_queue;
static _Thread_local array_count_t bogus_pos;

void retractable_init(void)
{
	retractable_heap_init(retractable_queue);
	// bogus entry to make sure retractable_queue is always non-empty
	struct lp_ctx *bogus = (struct lp_ctx *)((char *)&bogus_pos - offsetof(struct lp_ctx, retractable_entry_pos));
	const struct rq_elem rq = {.t = SIMTIME_MAX, .lp = bogus};
	retractable_heap_insert(retractable_queue, rq_elem_is_before, rq_elem_update, rq);
}

void retractable_lp_init(struct lp_ctx *lp)
{
	lp->retractable_ts_pointer = NULL;
}

void rs_retractable_enable(simtime_t *retractable_ts_pointer)
{
	assert(!current_lp->retractable_ts_pointer);
	current_lp->retractable_ts_pointer = retractable_ts_pointer;
	const struct rq_elem rq = {.t = SIMTIME_MAX, .lp = current_lp};
	retractable_heap_insert(retractable_queue, rq_elem_is_before, rq_elem_update, rq);
}

void retractable_fini(void)
{
	retractable_heap_fini(retractable_queue);
}

void retractable_reschedule(const struct lp_ctx *lp)
{
	if(!lp->retractable_ts_pointer)
		return;

	simtime_t t = *lp->retractable_ts_pointer;
	array_count_t pos = lp->retractable_entry_pos;
	struct rq_elem rq = array_get_at(retractable_queue, pos);
	if(t == rq.t)
		return;

	bool lowered = t > rq.t;
	rq.t = t;

	if(lowered)
		retractable_heap_priority_decreased(retractable_queue, rq_elem_is_before, rq_elem_update, rq, pos);
	else
		retractable_heap_priority_increased(retractable_queue, rq_elem_is_before, rq_elem_update, rq, pos);
}

struct lp_msg *retractable_extract(void)
{
	struct rq_elem rq = retractable_heap_min(retractable_queue);
	struct lp_msg *ret = msg_allocator_pack(rq.lp - lps, rq.t, LP_RETRACTABLE, NULL, 0);
	ret->raw_flags = 0;
	return ret;
}

bool retractable_is_before(const simtime_t normal_t)
{
	return retractable_heap_min(retractable_queue).t < normal_t;
}
