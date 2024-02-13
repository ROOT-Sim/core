#include <lp/lp.h>
#include <lp/process.h>
#include <lp/msg.h>
#include <mm/msg_allocator.h>
#include <datatypes/msg_queue.h>

#define mark_msg_remote(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) | 2U))
#define mark_msg_sent(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) | 1U))
#define unmark_msg_remote(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) - 2U))
#define unmark_msg_sent(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) - 1U))


void* get_lp_state_base_pointer(unsigned int i){
    return lps[i].state_pointer;
} 


void align_lp_state_to_gvt(simtime_t gvt, unsigned l){
	struct lp_ctx *lp = lps+l;
    array_count_t i = array_count(lp->p.p_msgs);
	if(!i) return;
    
    const struct lp_msg *msg;
    do{
		msg = array_get_at(lp->p.p_msgs, --i);
        
	} while(is_msg_sent(msg) || gvt < msg->dest_t);
	
    i++;
    do_rollback(lp, i);
	termination_on_lp_rollback(lp, gvt);
}


void custom_schedule_for_gpu(simtime_t gvt, unsigned sen, unsigned rec, simtime_t ts, unsigned type, void *pay, unsigned long size){
	//printf("B scheduling for %u a message from %u at %f\n", rec, sen, ts);
	if(gvt > ts) printf("GVT creeepy %lf %lf %lf\n", lps[rec].p.bound, ts, gvt);
	if(lps[rec].p.bound > ts) printf("BOUND creeepy %lf %lf %lf\n", lps[rec].p.bound, ts, gvt);
	struct lp_msg *msg = msg_allocator_pack(rec, ts, type, pay, size);
	msg->flags = 0U;
	msg_queue_insert(msg);
	//array_push((lps+sen)->p.p_msgs, mark_msg_sent(msg)); // TODO why this??
}
