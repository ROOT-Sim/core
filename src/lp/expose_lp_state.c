#include <lp/lp.h>
#include <lp/process.h>
#include <lp/msg.h>
#include <mm/msg_allocator.h>
#include <datatypes/msg_queue.h>
#include <gvt/termination.h>

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


extern int pack_and_insert_gpu_event(unsigned des_node, unsigned sen_node, int ts, unsigned type);

unsigned estimate_transfer_per_lp_events_without_filter(unsigned l){
	struct lp_ctx *lp = lps+l;
    return array_count(lp->p.p_msgs);
}


unsigned estimate_transfer_per_lp_events(unsigned l, simtime_t gvt){
	unsigned cnt = 0;
    struct lp_ctx *lp = lps+l;
    const struct lp_msg *msg;

    for(unsigned i=0;i<array_count(lp->p.p_msgs);i++){
		msg = array_get_at(lp->p.p_msgs, i);
        if(msg->dest_t < gvt) continue;
        if(msg->m_type == LP_REINIT) continue;
        if(msg->m_type == LP_INIT) continue;
        if(msg->m_type == LP_FINI) continue;
        if(is_msg_sent(msg)) continue;
        if(msg->flags & MSG_FLAG_ANTI) continue; 
        cnt++;
	}
    return cnt;
}

void clean_per_thread_queue(){
    msg_queue_count();
}


unsigned transfer_per_lp_events(unsigned l, simtime_t gvt){
	unsigned cnt = 0;
    struct lp_ctx *lp = lps+l;
    const struct lp_msg *msg;
    int res;
    for(unsigned i=0;i<array_count(lp->p.p_msgs);i++){
		msg = array_get_at(lp->p.p_msgs, i);
        if(msg->dest_t < gvt) continue;
        if(msg->m_type == LP_REINIT) continue;
        if(msg->m_type == LP_INIT) continue;
        if(msg->m_type == LP_FINI) continue;
        if(is_msg_sent(msg)) continue;
        if(msg->flags & MSG_FLAG_ANTI) continue; 
        
        cnt++;
        res = pack_and_insert_gpu_event(l, 0, (int)msg->dest_t, msg->m_type);
        if(!res){
            printf("failed to transfer all events %u\n", cnt);
            exit(1);
        }
	}
    return cnt;
}


int transfer_per_thread_event(struct lp_msg *msg, simtime_t gvt){
	int res;

    if(msg->dest_t < gvt) return 0;
    if(msg->m_type == LP_REINIT) return 0;
    if(msg->m_type == LP_INIT) return 0;
    if(msg->m_type == LP_FINI) return 0;
    if(msg->flags & MSG_FLAG_ANTI) return 0; 
        
    res = pack_and_insert_gpu_event(msg->dest, 0, (int)msg->dest_t, msg->m_type);
    if(!res){
        printf("%u: failed to transfer a per thread event\n", rid);
        exit(1);
    }
	return res;
}

int transfer_per_thread_events(simtime_t gvt){
   return msg_queue_iterate_over_msg(transfer_per_thread_event, gvt);    
}


void custom_schedule_from_gpu(simtime_t gvt, unsigned sen, unsigned rec, simtime_t ts, unsigned type, void *pay, unsigned long size){
	(void)sen;
	if(gvt > ts) printf("GVT creeepy %lf %lf %lf\n", lps[rec].p.bound, ts, gvt);
	if(lps[rec].p.bound > ts) printf("BOUND creeepy %lf %lf %lf\n", lps[rec].p.bound, ts, gvt);
	struct lp_msg *msg = msg_allocator_pack(rec, ts, type, pay, size);
	msg->flags = 0U;
	msg_queue_insert(msg);
}
