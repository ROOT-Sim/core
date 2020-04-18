#pragma once

#include <stdint.h>

extern void random_lib_lp_init(void);

extern double Random(void);
extern uint64_t RandomU64(void);
extern double Expent(double mean);
