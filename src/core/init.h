#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <core/core.h>
#include <log/log.h>

typedef struct {
	int verbosity;
	simtime_t termination_time;
	unsigned gvt_period; // expressed in microseconds
#ifndef NEUROME_SERIAL
	bool core_binding;
#endif
} simulation_configuration;

extern simulation_configuration global_config;

extern void init_args_parse(int argc, char **argv);
