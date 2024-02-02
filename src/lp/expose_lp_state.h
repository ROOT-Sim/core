#pragma once

void* get_lp_state_base_pointer(unsigned int i);
#define lid_to_rid(lp_id) ((rid_t)(((lp_id) - lid_node_first) * global_config.n_threads / n_lps_node))
extern uint64_t lid_node_first;
