#pragma once

#include "ROOT-Sim.h"

typedef enum ftl_phase{
    INIT,
    CHALLENGE,
    GPU,
    CPU,
    END,
} ftl_phase;

struct data_point_raw {
	double wall_s;
	double gvt;
};

extern volatile ftl_phase current_phase;

extern void follow_the_leader(simtime_t current_gvt);
extern void cpu_ended(void);
extern void gpu_ended(void);
extern unsigned sim_can_end(void);
extern void set_gpu_rid(unsigned);

extern double cmp_speeds(struct data_point_raw *a, int len_a, struct data_point_raw *b, int len_b);
