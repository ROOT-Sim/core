#pragma once

#include <stdint.h>
#include <core/core.h>

// this initializes the topology environment
extern void topology_global_init(void);

extern __attribute__ ((pure)) uint64_t RegionsCount(void);
extern __attribute__ ((pure)) uint64_t DirectionsCount(void);
extern __attribute__ ((pure)) uint64_t GetReceiver(uint64_t from, enum _direction_t direction);

extern uint64_t FindReceiver(void);
