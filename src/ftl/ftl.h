#pragma once

typedef enum ftl_phase{
    INIT,
    CHALLENGE,
    GPU,
    CPU,
    END,
} ftl_phase;


extern volatile ftl_phase current_phase;

extern void follow_the_leader(simtime_t);
extern void cpu_ended(void);
extern void gpu_ended(void);
extern unsigned sim_can_end(void);
extern void set_gpu_rid(unsigned);
