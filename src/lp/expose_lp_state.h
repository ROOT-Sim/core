#pragma once

void* get_lp_state_base_pointer(unsigned int i);
void custom_schedule_from_gpu(simtime_t gvt, unsigned sen, unsigned rec, simtime_t ts, unsigned type, void *pay, unsigned long size);
void align_lp_state_to_gvt(simtime_t gvt, unsigned i);

unsigned transfer_per_lp_events(unsigned l, simtime_t gvt);
unsigned transfer_per_thread_events(simtime_t gvt);

#define lid_to_rid(lp_id) ((rid_t)(((lp_id) - lid_node_first) * global_config.n_threads / n_lps_node))
extern uint64_t lid_node_first;


unsigned estimate_transfer_per_lp_events_without_filter(unsigned l);
unsigned estimate_transfer_per_lp_events(unsigned l, simtime_t gvt);
void clean_per_thread_queue();
