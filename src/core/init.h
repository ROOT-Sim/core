#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <log/log.h>

typedef struct {
	int verbosity;
//	simtime_t termination_time;
#ifndef NEUROME_SERIAL
	unsigned gvt_period;
#endif
} simulation_configuration;

extern simulation_configuration global_config;

extern void init_args_parse(int argc, char **argv);
