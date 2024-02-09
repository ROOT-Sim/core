#include <lp/lp.h>
#include <lp/msg.h>
#include <mm/msg_allocator.h>

#define mark_msg_remote(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) | 2U))
#define mark_msg_sent(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) | 1U))
#define unmark_msg_remote(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) - 2U))
#define unmark_msg_sent(msg_p) ((struct lp_msg *)(((uintptr_t)(msg_p)) - 1U))


void* get_lp_state_base_pointer(unsigned int i){
    return lps[i].state_pointer;
} 


void custom_schedule_for_gpu(unsigned sen, unsigned rec, simtime_t ts, unsigned type, void *pay, unsigned long size){
	struct lp_msg *msg = msg_allocator_pack(rec, ts, type, pay, size);
	msg->flags = 0U;
	msg_queue_insert(msg);
	//array_push((lps+sen)->p.p_msgs, mark_msg_sent(msg)); // TODO why this??
}
