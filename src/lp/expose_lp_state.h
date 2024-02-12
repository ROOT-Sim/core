#pragma once

void* get_lp_state_base_pointer(unsigned int i);
void custom_schedule_for_gpu(simtime_t gvt, unsigned sen, unsigned rec, simtime_t ts, unsigned type, void *pay, unsigned long size);

#define lid_to_rid(lp_id) ((rid_t)(((lp_id) - lid_node_first) * global_config.n_threads / n_lps_node))
extern uint64_t lid_node_first;
