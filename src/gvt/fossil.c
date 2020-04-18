#include <gvt/fossil.h>

#include <gvt/gvt.h>
#include <lp/lp.h>
#include <lp/process.h>
#include <mm/model_allocator.h>
#include <mm/msg_allocator.h>

#include <memory.h>

#define FOSSIL_CALLS 10

static __thread unsigned fossil_calls;

static inline void fossil_lp_collect(struct process_data *proc_p)
{
	array_count_t log_i = array_count(proc_p->logs) - 1;
	simtime_t gvt = current_gvt;

	while(
		array_get_at(proc_p->past_msgs,
			array_get_at(proc_p->logs, log_i).i_past_msg
		)->dest_t > gvt
	){
		--log_i;
	}

	if(!log_i)
		return;

	array_count_t past_i = array_get_at(proc_p->logs, log_i).i_past_msg;
	array_count_t j = array_count(proc_p->logs);
	while(j > log_i){
		--j;
		array_get_at(proc_p->logs, j).i_past_msg -= past_i;
	}

	while(j--){
		model_checkpoint_free(array_get_at(proc_p->logs, j).chkp);
	}
	array_truncate_first(proc_p->logs, log_i);

	array_count_t sent_i = array_count(proc_p->sent_msgs);
	j = array_count(proc_p->past_msgs);
	do{
		--sent_i;
		j -= array_get_at(proc_p->sent_msgs, sent_i) == NULL;
	} while(j > past_i);
	array_truncate_first(proc_p->sent_msgs, sent_i);

	while(j--){
		msg_allocator_free(array_get_at(proc_p->past_msgs, j));
	}
	array_truncate_first(proc_p->past_msgs, past_i);
}

void fossil_collect(void)
{
	fossil_calls++;
	if(fossil_calls < FOSSIL_CALLS)
		return;

	uint64_t i, lps_cnt;
	lps_iter_init(i, lps_cnt);

	while(lps_cnt--){
		fossil_lp_collect(&lps[i].p);
		i++;
	}

	msg_allocator_fossil_collect();

	fossil_calls = 0;
}
