#pragma once

struct curandStateXORWOW {
    unsigned int d, v[5];
    int boxmuller_flag;
    float boxmuller_extra;
    double boxmuller_extra_double;
};
typedef struct curandStateXORWOW curandStateXORWOW_t;
typedef curandStateXORWOW_t curandState_t;


void cpu_curand_init(unsigned long long seed, 
                                            unsigned long long subsequence, 
                                            unsigned long long offset, 
                                            curandStateXORWOW_t *state);
                                            
unsigned int cpu_curand(curandStateXORWOW_t *state);
unsigned int cpu_random(curandState_t *state, uint max);
float _cpu_curand_uniform(unsigned int x);
float cpu_curand_uniform(curandStateXORWOW_t *state);
unsigned int cpu_random_exp(curandState_t *state, uint mean);
