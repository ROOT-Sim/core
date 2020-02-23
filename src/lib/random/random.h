#pragma once

#include <stdint.h>

#ifndef __SIZEOF_INT128__
#error Unsupported platform
#endif

/// We use 128 bit numbers in random number generation
typedef __uint128_t uint128_t;

extern void random_init();


extern double Random(void);
extern uint64_t RandomU64(void);
