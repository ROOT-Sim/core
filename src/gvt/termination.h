#pragma once

struct termination_data {
	simtime_t terminated;
};

#define termination_can_exit()

extern void termination_lp_init();
extern void termination_processed_msg();
