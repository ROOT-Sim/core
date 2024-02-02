extern "C" {

#include "cpu_curand.h"
    
#define CURAND_2POW32_INV (2.3283064e-10f)
    
    
void cpu_curand_init(unsigned long long seed, 
                                            unsigned long long subsequence, 
                                            unsigned long long offset, 
                                            curandStateXORWOW_t *state)
{
    // Break up seed, apply salt
    unsigned int s0 = ((unsigned int)seed) ^ 0xaad26b49UL;
    unsigned int s1 = (unsigned int)(seed >> 32) ^ 0xf7dcefddUL;
    // Simple multiplication to mix up bits
    unsigned int t0 = 1099087573UL * s0;
    unsigned int t1 = 2591861531UL * s1;
    state->d = 6615241 + t1 + t0;
    state->v[0] = 123456789UL + t0;
    state->v[1] = 362436069UL ^ t0;
    state->v[2] = 521288629UL + t1;
    state->v[3] = 88675123UL ^ t1;
    state->v[4] = 5783321UL + t0;
    state->boxmuller_flag = 0;
}

unsigned int cpu_curand(curandStateXORWOW_t *state){
    unsigned int t;
    t = (state->v[0] ^ (state->v[0] >> 2));
    state->v[0] = state->v[1];
    state->v[1] = state->v[2];
    state->v[2] = state->v[3];
    state->v[3] = state->v[4];
    state->v[4] = (state->v[4] ^ (state->v[4] <<4)) ^ (t ^ (t << 1));
    state->d += 362437;
    return state->v[4] + state->d;
}


unsigned int cpu_random(curandState_t *state, uint max) {
	return cpu_curand(state) % max;
}

float _cpu_curand_uniform(unsigned int x){
    return x * CURAND_2POW32_INV + (CURAND_2POW32_INV/2.0f);
}

float cpu_curand_uniform(curandStateXORWOW_t *state){
    return _cpu_curand_uniform(cpu_curand(state));
}


unsigned int cpu_random_exp(curandState_t *state, uint mean) {
	float ru = cpu_curand_uniform(state);
	return -(mean * logf(ru));
}


}
