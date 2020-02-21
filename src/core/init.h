#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <log/log.h>

typedef struct {
	uint64_t lps_cnt;
	int verbosity;
#ifndef NEUROME_SERIAL
	unsigned threads_cnt;
	unsigned gvt_period;
#endif
} simulation_configuration;

extern simulation_configuration global_config;

extern void parse_args(int argc, char **argv);
