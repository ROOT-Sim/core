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
extern double gimme_current_time_please(void);

extern bool is_cpu_faster(void);
extern void register_cpu_data(double wall_s, double gvt);
extern void register_gpu_data(double wall_s, double gvt);


#define DEFAULT_FORECAST_STEPS 10
#define CHALLENGE_PERIODS 10
