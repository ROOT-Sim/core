/**
* @file core/init.c
*
* @brief Initialization routines
*
* This module implements the simulator initialization routines
*
* @copyright
* Copyright (C) 2008-2019 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*
* @author Francesco Quaglia
* @author Andrea Piccione
* @author Alessandro Pellegrini
* @author Roberto Vitali
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <float.h>
#include <sysexits.h>
#include <argp.h>
#include <errno.h>

#include <ROOT-Sim.h>
#include <arch/thread.h>
#include <communication/communication.h>
#include <core/core.h>
#include <core/init.h>
#include <datatypes/bitmap.h>
#include <scheduler/process.h>
#include <scheduler/ht_sched.h>
#include <gvt/gvt.h>
#include <gvt/ccgs.h>
#include <scheduler/scheduler.h>
#include <mm/state.h>
<<<<<<< HEAD
#include <mm/ecs.h>
#include <mm/mm.h>
=======
#include <mm/dymelor.h>
#include <mm/globvars.h>
#include <mm/malloc.h>
#include <core/backtrace.h> // Place this after malloc.h!
>>>>>>> origin/globvars
#include <statistics/statistics.h>
#include <lib/numerical.h>
#include <lib/topology.h>
#include <lib/abm_layer.h>
#include <serial/serial.h>
#include <core/power.h>
#ifdef HAVE_MPI
#include <communication/mpi.h>
#endif

<<<<<<< HEAD

/// This is the list of mnemonics for arguments
enum _opt_codes{
	OPT_FIRST = 128, /**< this is used as an offset to the enum values, so that argp doesn't assign short options */

	// make sure these ones are mapped correctly to the external enum param_codes,
	OPT_SCHEDULER = 	OPT_FIRST + PARAM_SCHEDULER,
	OPT_CKTRM_MODE = 	OPT_FIRST + PARAM_CKTRM_MODE,
	OPT_LPS_DISTRIBUTION =	OPT_FIRST + PARAM_LPS_DISTRIBUTION,
	OPT_VERBOSE = 		OPT_FIRST + PARAM_VERBOSE,
	OPT_STATS = 		OPT_FIRST + PARAM_STATS,
	OPT_STATE_SAVING = 	OPT_FIRST + PARAM_STATE_SAVING,
	OPT_SNAPSHOT = 		OPT_FIRST + PARAM_SNAPSHOT,

	OPT_NP,
	OPT_NPRC,
	OPT_OUTPUT_DIR,
	OPT_NPWD,
	OPT_P,
	OPT_FULL,
	OPT_SOFTINC,
	OPT_HARDINC,
	OPT_A,
	OPT_GVT,
	OPT_SIMULATION_TIME,
	OPT_WALLCLOCK_TIME,
	OPT_DETERMINISTIC_SEED,
	OPT_SEED,
	OPT_SERIAL,
	OPT_NO_CORE_BINDING,
<<<<<<< HEAD
=======
	OPT_CONTROLLERS,

#ifdef HAVE_PREEMPTION
>>>>>>> origin/asym
	OPT_PREEMPTION,
<<<<<<< HEAD
	OPT_SILENT,
<<<<<<< HEAD
=======
	OPT_SLAB_MSG_SIZE,
>>>>>>> origin/PADS2020
=======
	OPT_POWERCAP,
	OPT_POWERCAP_EXPLORE,
>>>>>>> origin/energy
	OPT_LAST
};

// XXX we offset the first level with OPT_FIRST so remember about it when you index it!
const char * const param_to_text[][5] = {
	[OPT_SCHEDULER - OPT_FIRST] = {
			[SCHEDULER_INVALID] = "invalid scheduler",
			[SCHEDULER_STF] = "stf",
            [BATCH_LOWEST_TIMESTAMP] ="batch"
	},
	[OPT_CKTRM_MODE - OPT_FIRST] = {
			[CKTRM_INVALID] = "invalid termination checking",
			[CKTRM_NORMAL] = "normal",
			[CKTRM_INCREMENTAL] = "incremental",
			[CKTRM_ACCURATE] = "accurate"
	},
	[OPT_LPS_DISTRIBUTION - OPT_FIRST] = {
			[LP_DISTRIBUTION_INVALID] = "invalid LPs distribution",
			[LP_DISTRIBUTION_BLOCK] = "block",
			[LP_DISTRIBUTION_CIRCULAR] = "circular"
	},
	[OPT_VERBOSE - OPT_FIRST] = {
			[VERBOSE_INVALID] = "invalid verbose specification",
			[VERBOSE_INFO] = "info",
			[VERBOSE_DEBUG] = "debug",
			[VERBOSE_NO] = "no"
	},
	[OPT_STATS - OPT_FIRST] = {
			[STATS_INVALID] = "invalid statistics specification",
			[STATS_GLOBAL] = "global",
			[STATS_PERF] = "performance",
			[STATS_LP] = "lp",
			[STATS_ALL] = "all"
	},
	[OPT_STATE_SAVING - OPT_FIRST] = {
			[STATE_SAVING_INVALID] = "invalid checkpointing specification",
			[STATE_SAVING_COPY] = "copy",
			[STATE_SAVING_PERIODIC] = "periodic",
	},
	[OPT_SNAPSHOT - OPT_FIRST] = {
			[SNAPSHOT_INVALID] = "invalid snapshot specification",
			[SNAPSHOT_FULL] = "full",
			[SNAPSHOT_SOFTINC] = "software based incremental",
			[SNAPSHOT_HARDINC] = "hardware based incremental",
	}
};

const char *argp_program_version 	= PACKAGE_STRING "\nCopyright (C) 2008-2019 HPDCS Group";
const char *argp_program_bug_address 	= PACKAGE_BUGREPORT;

// Directly from argp documentation:
// If non-zero, a string containing extra text to be printed before and after the options in a long help message,
// with the two sections separated by a vertical tab ('\v', '\013') character.
// By convention, the documentation before the options is just a short string explaining what the program does.
// Documentation printed after the options describe behavior in more detail.
static char doc[] = "ROOT-Sim - a fast distributed multithreaded Parallel Discrete Event Simulator \v For more information check the official wiki at https://github.com/HPDCS/ROOT-Sim/wiki";

// this isn't needed (we haven't got non option arguments to document)
static char args_doc[] = "";
// the options recognized by argp
static const struct argp_option argp_options[] = {
	{"wt",			OPT_NP,			"VALUE",	0,		"Number of total cores being used by the simulation", 0},
	{"lp",			OPT_NPRC,		"VALUE",	0,		"Total number of Logical Processes being launched at simulation startup", 0},
	{"output-dir",		OPT_OUTPUT_DIR,		"PATH",		0,		"Path to a folder where execution statistics are stored. If not present, it is created", 0},
	{"scheduler",		OPT_SCHEDULER,		"TYPE",		0,		"LP Scheduling algorithm. Supported values are: stf", 0},
	{"npwd",		OPT_NPWD,		0,		0,		"Non Piece-Wise-Deterministic simulation model. See manpage for accurate description", 0},
	{"p",			OPT_P,			"VALUE",	0,		"Checkpointing interval", 0},
	{"full",		OPT_FULL,		0,		0,		"Take only full logs", 0},
	{"soft-inc",		OPT_SOFTINC,		0,		0,		"Take only software-aided incremental logs", 0},
	{"hard-inc",		OPT_HARDINC,		0,		0,		"Take only hardware-aided incremental logs", 0},
	{"A",			OPT_A,			0,		0,		"Autonomic subsystem: set checkpointing interval and log mode automatically at runtime (still to be released)", 0},
	{"gvt",			OPT_GVT,		"VALUE",	0,		"Time between two GVT reductions (in milliseconds)", 0},
	{"cktrm-mode",		OPT_CKTRM_MODE,		"TYPE",		0,		"Termination Detection mode. Supported values: normal, incremental, accurate", 0},
	{"simulation-time",	OPT_SIMULATION_TIME, 	"VALUE",	0,		"Halt the simulation when all LPs reach this logical time. 0 means infinite", 0},
	{"wallclock-time",	OPT_WALLCLOCK_TIME, 	"VALUE",	0,		"Halt the simulation when the wall clock time reaches this value. 0 means infinite", 0},
	{"lps-distribution",	OPT_LPS_DISTRIBUTION, 	"TYPE",		0,		"LPs distributions over simulation kernels policies. Supported values: block, circular", 0},
	{"deterministic-seed",	OPT_DETERMINISTIC_SEED,	0,		0, 		"Do not change the initial random seed for LPs. Enforces different deterministic simulation runs", 0},
	{"verbose",		OPT_VERBOSE,		"TYPE",		0,		"Verbose execution", 0},
	{"stats",		OPT_STATS,		"TYPE",		0,		"Level of detail in the output statistics", 0},
	{"seed",		OPT_SEED,		"VALUE",	0,		"Manually specify the initial random seed", 0},
	{"serial",		OPT_SERIAL,		0,		0,		"Run a serial simulation (using Calendar Queues)", 0},
	{"sequential",		OPT_SERIAL,		0,		OPTION_ALIAS,	NULL, 0},
	{"no-core-binding",	OPT_NO_CORE_BINDING,	0,		0,		"Disable the binding of threads to specific physical processing cores", 0},
<<<<<<< HEAD
	{"silent-output",	OPT_SILENT,		0,		0,		"Disable any output generated by printf() calls (also from models)", 0},
=======
	{"ncontrollers",	OPT_CONTROLLERS,	"VALUE",		0,		"Initial number of Controller Threads", 0},


#ifdef HAVE_PREEMPTION
>>>>>>> origin/asym
	{"no-preemption",	OPT_PREEMPTION,		0,		0,		"Disable Preemptive Time Warp", 0},
<<<<<<< HEAD
	{"slab-msg-size",	OPT_SLAB_MSG_SIZE,	"VALUE",	0,		"Sets the Slab allocator message size", 0},
=======
	{"powercap",		OPT_POWERCAP,		"VALUE",	0,		"Power capping value (in Watts) which defines a limit for the system power consumption", 0},
	{"powercap-exploration",OPT_POWERCAP_EXPLORE,	"VALUE",	0,		"Power Capping Exploration Strategy", 0},
>>>>>>> origin/energy
	{0}
};
=======
/// The initial number of application-level argument the simulator reserves space for. If a greater number is found, the array is realloc'd
#define APPLICATION_ARGUMENTS 32

// This is the list of mnemonics for arguments
#define OPT_NP			1
#define OPT_NPRC		2
#define OPT_OUTPUT_DIR		3
#define OPT_SCHEDULER		4
#define OPT_NPWD		5
#define OPT_P			6
#define OPT_FULL		7
#define OPT_INC			8
#define OPT_A			9
#define OPT_GVT			10
#define OPT_CKTRM_MODE		11
#define OPT_BLOCKING_GVT	12
#define OPT_GVT_SNAPSHOT_CYCLES	13
#define OPT_SIMULATION_TIME	14
#define OPT_LPS_DISTRIBUTION	15
#define OPT_DETERMINISTIC_SEED	16
#define OPT_VERBOSE		17
#define OPT_STATS		18
#define OPT_SEED		19
#define OPT_SERIAL		20
#define OPT_NO_CORE_BINDING	21
#define OPT_CONTROLLERS		23
#define OPT_CONTROLLERS_FREQ	24
#define OPT_POWERCAP		25
#define OPT_POWERCAP_EXPLORE	26

#ifdef HAVE_PARALLEL_ALLOCATOR
#define OPT_ALLOCATOR		27
#endif

#ifdef HAVE_PREEMPTION
#define OPT_PREEMPTION		28
#endif

/// This variable keeps the executable's name
char *program_name;

/// To let the parallel initialization access the user-level command line arguments
struct app_arguments model_parameters;


static char *opt_desc[] = {
	"",
	"Number of total cores being used by the simulation",
	"Total number of Logical Processes being lunched at simulation startup",
	"Path to a folder where execution statistics are stored. If not present, it is created",
	"LP Scheduling algorithm. Supported values are: stf",
	"Non Piece-Wise-Deterministic simulation model. See manpage for accurate description",
	"Checkpointing interval",
	"Take only full logs",
	"Take only incremental logs (still to be released)",
	"Autonomic subsystem: set checkpointing interval and log mode automatically at runtime (still to be released)",
	"Time between two GVT reductions (in milliseconds)",
	"Termination Detection mode. Supported values: standard, incremental",
	"Blocking GVT. All distributed nodes block until a consensus is agreed",
	"Termination detection is invoked after this number of GVT reductions",
	"Halt the simulation when all LPs reach this logical time. 0 means infinite",
	"LPs distributions over simulation kernels policies. Supported values: block, circular",
	"Do not change the initial random seed for LPs. Enforces different deterministic simulation runs",
	"Verbose execution",
	"Level of detail in the output statistics",
	"Manually specify the initial random seed",
	"Run a serial simulation (using Calendar Queues)",
	"Disable the binding of threads to specific phisical processing cores",
	"Initial number of Controller Threads",
	"Initial frequency of Controller Threads",
	"Power value (in Watts) for Power Capping",
	"Power Capping Exploration Strategy",
>>>>>>> origin/power

#define malformed_option_failure()	argp_error(state, "invalid value \"%s\" in %s option.\nAborting!", arg, state->argv[state->next -1 -(arg != NULL)])

#define conflicting_option_failure(msg)	argp_error(state, "the requested option %s with value \"%s\" is conflicting: " msg "\nAborting!", state->argv[state->next -1 -(arg != NULL)], arg)

// this parses an string option leveraging the 2d array of strings specified earlier
// the weird iteration style skips the element 0, which we know is associated with an invalid value description
#define handle_string_option(label, var)						\
	case label:									\
	({										\
		unsigned __i = 1;							\
		while(1) {								\
			if(strcmp(arg, param_to_text[key - OPT_FIRST][__i]) == 0) {	\
				var = __i;						\
				break;							\
			}								\
			if(!param_to_text[key - OPT_FIRST][++__i])			\
				malformed_option_failure();				\
		}									\
	});										\
	break


// the compound expression equivalent to __value >= low is needed in order to suppress a warning when low == 0
#define parse_ullong_limits(low, high) 	\
	({														\
		unsigned long long int __value;										\
		char *__endptr;												\
		__value = strtoull(arg, &__endptr, 10);									\
		if(!(*arg != '\0' && *__endptr == '\0' && (__value > low || __value == low) && __value <= high)) {	\
			malformed_option_failure();									\
		}													\
		__value;												\
	})

#define parse_ldouble_limits(low, high) 	\
	({														\
		long double __value;										\
		char *__endptr;												\
		__value = strtold(arg, &__endptr);									\
		if(!(*arg != '\0' && *__endptr == '\0' && __value >= low && __value <= high)) {	\
			malformed_option_failure();									\
		}													\
		__value;												\
	})

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	// this is used in order to ensure that the user doesn't use duplicate options
	static rootsim_bitmap scanned[bitmap_required_size(OPT_LAST - OPT_FIRST)];

	if(key >= OPT_FIRST && key < OPT_LAST){
		if(bitmap_check(scanned, key - OPT_FIRST))
			conflicting_option_failure("this option has already been specified");

		bitmap_set(scanned, key - OPT_FIRST);
	}

<<<<<<< HEAD
	switch (key) {
		case OPT_NP:
			if(strcmp(arg, "auto") == 0) {
				n_cores = get_cores();
			} else {
				active_threads = n_cores = parse_ullong_limits(1, UINT_MAX);
			}
			break;
=======
#ifdef HAVE_PREEMPTION
	"Disable Preemptive Time Warp",
#endif
	""
};
>>>>>>> origin/power

		case OPT_NPRC:
            n_LP_tot = parse_ullong_limits(1, UINT_MAX);
			break;

<<<<<<< HEAD
		case OPT_OUTPUT_DIR:
			rootsim_config.output_dir = arg;
			break;
		
		case OPT_SLAB_MSG_SIZE:
			rootsim_config.slab_msg_size = parse_ullong_limits(512, 4096);
			break;

		handle_string_option(OPT_SCHEDULER, rootsim_config.scheduler);
		handle_string_option(OPT_FULL, rootsim_config.snapshot);
		handle_string_option(OPT_CKTRM_MODE, rootsim_config.check_termination_mode);
		handle_string_option(OPT_VERBOSE, rootsim_config.verbose);
		handle_string_option(OPT_STATS, rootsim_config.stats);
		handle_string_option(OPT_LPS_DISTRIBUTION, rootsim_config.lps_distribution);

		case OPT_NPWD:
			if (bitmap_check(scanned, OPT_P-OPT_FIRST)) {
				conflicting_option_failure("I'm requested to run non piece-wise deterministically, but a checkpointing interval is set already.");
			} else {
				rootsim_config.checkpointing = STATE_SAVING_COPY;
			}
			break;
=======
static struct option long_options[] = {
	{"np",			required_argument,	0, OPT_NP},
	{"output-dir",		required_argument,	0, OPT_OUTPUT_DIR},
	{"scheduler",		required_argument,	0, OPT_SCHEDULER},
	{"npwd",		no_argument,		0, OPT_NPWD},
	{"p",			required_argument,	0, OPT_P},
	{"full",		no_argument,		0, OPT_FULL},
	{"inc",			no_argument,		0, OPT_INC},
	{"A",			no_argument,		0, OPT_A},
	{"gvt",			required_argument,	0, OPT_GVT},
	{"cktrm_mode",		required_argument,	0, OPT_CKTRM_MODE},
	{"nprc",		required_argument,	0, OPT_NPRC},
	{"blocking_gvt",	no_argument,		0, OPT_BLOCKING_GVT},
	{"gvt_snapshot_cycles",	required_argument,	0, OPT_GVT_SNAPSHOT_CYCLES},
	{"simulation_time",	required_argument,	0, OPT_SIMULATION_TIME},
	{"lps_distribution",	required_argument,	0, OPT_LPS_DISTRIBUTION},
	{"deterministic_seed",	no_argument,		0, OPT_DETERMINISTIC_SEED},
	{"verbose",		required_argument,	0, OPT_VERBOSE},
	{"stats",		required_argument,	0, OPT_STATS},
	{"seed",		required_argument,	0, OPT_SEED},
	{"no-core-binding",	no_argument,		0, OPT_NO_CORE_BINDING},
	{"ncontrollers",	required_argument,	0, OPT_CONTROLLERS},
	{"controllers-freq",	required_argument,	0, OPT_CONTROLLERS_FREQ},
	{"powercap",		required_argument,	0, OPT_POWERCAP},
	{"powercap-exploration",required_argument,	0, OPT_POWERCAP_EXPLORE},
>>>>>>> origin/power

		case OPT_P:
			if(bitmap_check(scanned, OPT_NPWD-OPT_FIRST)) {
				conflicting_option_failure("Copy State Saving is selected, but I'm requested to set a checkpointing interval.");
			} else {
				rootsim_config.checkpointing = STATE_SAVING_PERIODIC;
				rootsim_config.ckpt_period = parse_ullong_limits(1, 40);
				// This is a micro optimization that makes the LogState function avoid checking the checkpointing interval and keeping track of the logs taken
				if(rootsim_config.ckpt_period == 1)
					rootsim_config.checkpointing = STATE_SAVING_COPY;
			}
			break;

<<<<<<< HEAD
		case OPT_SOFTINC:
			rootsim_config.snapshot = SNAPSHOT_SOFTINC;
			break;
=======
#ifdef HAVE_PREEMPTION
	{"no-preemption",	no_argument,		0, OPT_PREEMPTION},
#endif
	{"serial",		no_argument,		0, OPT_SERIAL},
	{"sequential",		no_argument,		0, OPT_SERIAL},

	{0,			0,			0, 0}
};
>>>>>>> origin/power

		case OPT_HARDINC:
			rootsim_config.snapshot = SNAPSHOT_HARDINC;
			break;

		case OPT_A:
			argp_failure(state, EXIT_FAILURE, ENOSYS, "autonomic state saving is not supported in stable version yet.\nAborting...");
			break;

		case OPT_GVT:
			rootsim_config.gvt_time_period = parse_ullong_limits(1, 10000);
			break;

		case OPT_SIMULATION_TIME:
			rootsim_config.simulation_time = parse_ullong_limits(0, INT_MAX);
			break;

		case OPT_WALLCLOCK_TIME:
			rootsim_config.wallclock_time = parse_ullong_limits(0, INT_MAX);
			break;

		case OPT_DETERMINISTIC_SEED:
			rootsim_config.deterministic_seed = true;
			break;

		case OPT_SEED:
			rootsim_config.set_seed = parse_ullong_limits(0, UINT64_MAX);
			break;

		case OPT_SERIAL:
			rootsim_config.serial = true;
			break;

		case OPT_NO_CORE_BINDING:
			rootsim_config.core_binding = false;
			break;

<<<<<<< HEAD
<<<<<<< HEAD
=======
		case OPT_CONTROLLERS:
			rootsim_config.num_controllers = parse_ullong_limits(0,INT_MAX);
			break;

#ifdef HAVE_PREEMPTION
>>>>>>> origin/asym
		case OPT_PREEMPTION:
			rootsim_config.disable_preemption = true;
			break;
=======
		#define parseIntLimits(s, low, high) ({\
					int period;\
					char *endptr;\
					period = strtol(s, &endptr, 10);\
					if(!(*s != '\0' && *endptr == '\0' && period >= low && period <= high)) {\
						rootsim_error(true, "Invalid option value: %s\n", s);\
					}\
					period;\
				     })

		switch (c) {

			case OPT_NP:
				n_cores = parseInt(optarg);
				// TODO: Qui si dovra' a un certo punto spostare questo controllo nel mapping LP <-> Kernel <-> Thread!!!
				if(n_cores > get_cores()) {
					rootsim_error(true, "Demanding %d cores, which are more than available (%d)\n", n_cores, get_cores());
					return -1;
				}
				if(n_cores <= 0) {
					rootsim_error(true, "Demanding a non-positive number of cores\n");
					return -1;
				}

				if(n_cores > MAX_THREADS_PER_KERNEL){
					rootsim_error(true, "Too many threads, maximum supported number is %u\n", MAX_THREADS_PER_KERNEL);
				}
				break;

			case OPT_OUTPUT_DIR:
				length = strlen(optarg);
				rootsim_config.output_dir = (char *)rsalloc(length + 1);
				strcpy(rootsim_config.output_dir, optarg);
				break;

			case OPT_SCHEDULER:
				if(strcmp(optarg, "stf") == 0) {
					rootsim_config.scheduler = SMALLEST_TIMESTAMP_FIRST;
				} else if(strcmp(optarg, "batch") == 0){
					rootsim_config.scheduler = BATCH_LOWEST_TIMESTAMP;
				} else {
					rootsim_error(true, "Invalid argument for scheduler parameter\n");
					return -1;
				}
				break;

			case OPT_NPWD:
				if (rootsim_config.checkpointing == INVALID_STATE_SAVING) {
					rootsim_config.checkpointing = COPY_STATE_SAVING;
				} else {
					rootsim_error(false, "Some options are conflicting: I'm requested to run non piece-wise deterministically, but a checkpointing interval is set. Skipping the -npwd option.\n");
				}
				break;

			case OPT_P:
				if(rootsim_config.checkpointing == COPY_STATE_SAVING) {
					rootsim_error(false, "Some options are conflicting: Copy State Saving is selected, but I'm requested to set a checkpointing interval. Skipping the -p option.\n");
				} else {
					rootsim_config.checkpointing = PERIODIC_STATE_SAVING;
					rootsim_config.ckpt_period = parseIntLimits(optarg, 1, 40);
					if(rootsim_config.ckpt_period == 1) {
						rootsim_config.checkpointing = COPY_STATE_SAVING;
					}
				}
				break;

			case OPT_FULL:
				if (rootsim_config.snapshot == INVALID_SNAPSHOT) {
					rootsim_config.snapshot = FULL_SNAPSHOT;
				}
				break;

			case OPT_INC:
				rootsim_error(false, "Incremental state saving is not supported in stable version yet...\n");
				break;

			case OPT_A:
				rootsim_error(false, "Autonomic state saving is not supported in stable version yet...\n");
				break;

			case OPT_GVT:
				rootsim_config.gvt_time_period = parseIntLimits(optarg, 1, INT_MAX);
				break;

			case OPT_CKTRM_MODE:
				if(strcmp(optarg, "standard") == 0) {
					rootsim_config.check_termination_mode = NORM_CKTRM;
				} else if(strcmp(optarg, "incremental") == 0) {
					rootsim_config.check_termination_mode = INCR_CKTRM;
				} else {
					rootsim_error(true, "Invalid argument for cktrm_mode\n");
					return -1;
				}
				break;

			case OPT_NPRC:
				n_prc_tot = parseIntLimits(optarg, 1, MAX_LPs); // In this way, a change in MAX_LPs is reflected here
				break;

			case OPT_BLOCKING_GVT:
				rootsim_config.blocking_gvt = true;
				break;

			case OPT_GVT_SNAPSHOT_CYCLES:
				rootsim_config.gvt_snapshot_cycles = parseIntLimits(optarg, 1, INT_MAX);
				break;

			case OPT_SIMULATION_TIME:
				rootsim_config.simulation_time = parseIntLimits(optarg, 0, INT_MAX);
				break;

			case OPT_LPS_DISTRIBUTION:
				if(strcmp(optarg, "block") == 0) {
					rootsim_config.lps_distribution = LP_DISTRIBUTION_BLOCK;
				} else if(strcmp(optarg, "circular") == 0) {
					rootsim_config.lps_distribution = LP_DISTRIBUTION_CIRCULAR;
				} else {
					rootsim_error(true, "Invalid argument for lps_distribution\n");
					return -1;
				}
				break;

			case OPT_DETERMINISTIC_SEED:
				rootsim_config.deterministic_seed = true;
				break;

			case OPT_VERBOSE:
				if(strcmp(optarg, "info") == 0) {
					rootsim_config.verbose = VERBOSE_INFO;
				} else if(strcmp(optarg, "debug") == 0) {
					rootsim_config.verbose = VERBOSE_DEBUG;
				} else if(strcmp(optarg, "no") == 0) {
					rootsim_config.verbose = VERBOSE_NO;
				} else {
					rootsim_error(true, "Invalid argument for verbose\n");
					return -1;
				}
				break;

			case OPT_STATS:
				if(strcmp(optarg, "all") == 0) {
					rootsim_config.stats = STATS_ALL;
				} else if(strcmp(optarg, "performance") == 0) {
					rootsim_config.stats = STATS_PERF;
				} else if(strcmp(optarg, "lp") == 0) {
					rootsim_config.stats = STATS_LP;
				} else if(strcmp(optarg, "global") == 0) {
					rootsim_config.stats = STATS_GLOBAL;
				} else {
					rootsim_error(true, "Invalid argument for stats\n");
					return -1;
				}
				break;

			case OPT_SEED:
				rootsim_config.set_seed = parseInt(optarg);
				break;

			case OPT_SERIAL:
				rootsim_config.serial = true;
				break;

			case OPT_NO_CORE_BINDING:
				rootsim_config.core_binding = false;
				break;

			case OPT_CONTROLLERS:
				rootsim_config.num_controllers = parseInt(optarg);
				break;

			case OPT_CONTROLLERS_FREQ:
				rootsim_config.controllers_freq = parseDouble(optarg);
				break;

			case OPT_POWERCAP:
				rootsim_config.powercap = parseDouble(optarg);
				break;

			case OPT_POWERCAP_EXPLORE:
				rootsim_config.powercap_exploration = parseInt(optarg);
				break;


			#ifdef HAVE_PREEMPTION
			case OPT_PREEMPTION:
				rootsim_config.disable_preemption = true;
				break;
			#endif
>>>>>>> origin/power

		case OPT_SILENT:
			rootsim_config.silent_output = true;
			break;

		case OPT_POWERCAP:
			rootsim_config.powercap = parse_ldouble_limits(0.0, LDBL_MAX);
			#ifndef HAVE_POWER_MANAGEMENT
			if(rootsim_config.powercap > 0.0) {
				conflicting_option_failure("Power Capping support not available at compile time");
			}
			#endif
			break;

		case OPT_POWERCAP_EXPLORE:
			rootsim_config.powercap_exploration = parse_ullong_limits(0, UINT64_MAX);
			break;

		case ARGP_KEY_INIT:

			memset(&rootsim_config, 0, sizeof(rootsim_config));
			memset(scanned, 0, sizeof(scanned));
			// Store the predefined values, before reading any overriding one
			rootsim_config.output_dir = DEFAULT_OUTPUT_DIR;
			rootsim_config.scheduler = SCHEDULER_STF;
			rootsim_config.lps_distribution = LP_DISTRIBUTION_BLOCK;
			rootsim_config.check_termination_mode = CKTRM_NORMAL;
			rootsim_config.stats = STATS_ALL;
			rootsim_config.verbose = VERBOSE_INFO;
			rootsim_config.snapshot = SNAPSHOT_FULL; // TODO: in the future, default to AUTONOMIC_
			rootsim_config.checkpointing = STATE_SAVING_PERIODIC;
			rootsim_config.gvt_time_period = 1000;
			rootsim_config.ckpt_period = 10;
			rootsim_config.simulation_time = 0;
			rootsim_config.deterministic_seed = false;
			rootsim_config.set_seed = 0;
			rootsim_config.serial = false;
			rootsim_config.core_binding = true;
			rootsim_config.disable_preemption = false;
<<<<<<< HEAD
			rootsim_config.silent_output = false;
<<<<<<< HEAD
=======
			rootsim_config.slab_msg_size = 512;
>>>>>>> origin/PADS2020
=======
			rootsim_config.powercap = 0.0;
			rootsim_config.powercap_exploration = 0;
>>>>>>> origin/energy
			break;

		case ARGP_KEY_SUCCESS:
			// TODO: we have to make sure that setting serial unsets all parallel related options since the serial runtime
			// relies on common code and default values
			if(rootsim_config.serial && rootsim_config.snapshot != SNAPSHOT_FULL){
				rootsim_error(false, "Running a serial simulation, resetting SNAPSHOT setting");
				rootsim_config.snapshot = SNAPSHOT_FULL;
			}

#ifndef HAVE_PMU
			if(rootsim_config.snapshot == SNAPSHOT_HARDINC)
				rootsim_error(true, "ROOT-Sim has been built without hardware incremental support");
#endif

			// sanity checks
			if(!rootsim_config.serial && !bitmap_check(scanned, OPT_NP - OPT_FIRST))
				rootsim_error(true, "Number of cores was not provided \"--wt\"\n");

			if(!bitmap_check(scanned, OPT_NPRC - OPT_FIRST))
				rootsim_error(true, "Number of LPs was not provided \"--lp\"\n");

			if(n_cores > get_cores())
				rootsim_error(true, "Demanding %u cores, which are more than available (%d)\n", n_cores, get_cores());

			if(n_cores > MAX_THREADS_PER_KERNEL)
				rootsim_error(true, "Too many threads, maximum supported number is %u\n", MAX_THREADS_PER_KERNEL);

			if(n_LP_tot > MAX_LPs)
				rootsim_error(true, "Too many LPs, maximum supported number is %u\n", MAX_LPs);

			if(!rootsim_config.serial && n_LP_tot < n_cores)
				rootsim_error(true, "Requested a simulation run with %u LPs and %u worker threads: the mapping is not possible\n", n_LP_tot, n_cores);

			print_config();

			break;
			
			/* these functionalities are not needed
		case ARGP_KEY_ARGS:
		case ARGP_KEY_NO_ARGS:
		case ARGP_KEY_SUCCESS:
		case ARGP_KEY_END:
		case ARGP_KEY_ARG:
		case ARGP_KEY_ERROR:
			*/
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

#undef parse_ullong_limits
#undef handle_string_option
#undef conflicting_option_failure
#undef malformed_option_failure

static struct argp_child argp_child[2] = {
		{0, 0, "Model specific options", 0},
		{0}
};

static struct argp argp = { argp_options, parse_opt, args_doc, doc, argp_child, 0, 0 };


/**
* This function initializes the simulator
*
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @param argc number of parameters passed at command line
* @param argv array of parameters passed at command line
*/
void SystemInit(int argc, char **argv)
{
#ifdef HAVE_MPI
	mpi_init(&argc, &argv);

	if(n_ker > MAX_KERNELS){
		rootsim_error(true, "Too many kernels, maximum supported number is %u\n", MAX_KERNELS);
	}
#else
	n_ker = 1;
#endif

	// Early initialization of ECS subsystem if needed
#ifdef HAVE_CROSS_STATE
	ecs_init();
#endif

	// this retrieves the model's argp parser if declared by the developer
	argp_child[0].argp = &model_argp;

	argp_parse (&argp, argc, argv, 0, NULL, NULL);

	// If we're going to run a serial simulation, configure the simulation to support it
	if(rootsim_config.serial) {
		ScheduleNewEvent = SerialScheduleNewEvent;
		initialize_lps();
		numerical_init();
		statistics_init();
		serial_init();
		topology_init();
		return;
	} else {
		ScheduleNewEvent = ParallelScheduleNewEvent;
	}

<<<<<<< HEAD
=======
	// If power management is enabled, initialize it 
	#ifdef POWER_MODULE
	init_powercap_module();
	#endif

	if (master_kernel()) {

		 printf("****************************\n"
			"*  ROOT-Sim Configuration  *\n"
			"****************************\n"
			"Kernels: %u\n"
			"Cores: %ld available, %d used\n"
			"Controllers: %d\n"
			"Number of Logical Processes: %u\n"
			"Output Statistics Directory: %s\n"
			"Scheduler: %d\n"
			#ifdef HAVE_MPI
			"MPI multithread support: %s\n"
			#endif
			"GVT Time Period: %.2f seconds\n"
			"Checkpointing Type: %d\n"
			"Checkpointing Period: %d\n"
			"Snapshot Reconstruction Type: %d\n"
			"Halt Simulation After: %d\n"
			"LPs Distribution Mode across Kernels: %d\n"
			"Check Termination Mode: %d\n"
			"Blocking GVT: %d\n"
			"Set Seed: %ld\n"
			"Power Capping: %fW\n",
			n_ker,
			get_cores(),
			n_cores,
			rootsim_config.num_controllers,
			n_prc_tot,
			rootsim_config.output_dir,
			rootsim_config.scheduler,
			#ifdef HAVE_MPI
			((mpi_support_multithread)? "yes":"no"),
			#endif
			rootsim_config.gvt_time_period / 1000.0,
			rootsim_config.checkpointing,
			rootsim_config.ckpt_period,
			rootsim_config.snapshot,
			rootsim_config.simulation_time,
			rootsim_config.lps_distribution,
			rootsim_config.check_termination_mode,
			rootsim_config.blocking_gvt,
			rootsim_config.set_seed,
			rootsim_config.powercap);
	}

	distribute_lps_on_kernels();

>>>>>>> origin/power
	// Initialize ROOT-Sim subsystems.
	// All init routines are executed serially (there is no notion of threads in there)
	// and the order of invocation can matter!
	base_init();
<<<<<<< HEAD
<<<<<<< HEAD
=======
	ht_sched_init();
	segment_init();
>>>>>>> origin/incremental
	initialize_lps();
	remote_memory_init();
=======
	threads_init();
	scheduler_init();
>>>>>>> origin/power
	statistics_init();
	scheduler_init();
	globvars_init();
	communication_init();
	threads_init();
	gvt_init();
	numerical_init();
	topology_init();

	// This call tells the simulation engine that the sequential initial simulation is complete
	initialization_complete();
}


/**
* Expose to the simulation model the internal configuration of the simulator, to
* allow fine-tuning of the model depending on the current runtime configuration
* and/or command-line parameters.
*
* @author Alessandro Pellegrini
*
* @param which the capability which the model is querying for availability
* @param info if this parameter is non-NULL, additional information is sent back
* 		to the application in the appropriate corresponding field. If this
* 		member is non-NULL, the @c capability member is set to @c which value,
* 		to let the model know what member of the union is meaningful.
*/
bool CapabilityAvailable(enum capability_t which, struct capability_info_t *info)
{

	// Early evaluate things that don't have any information to provide
	switch(which) {
		case CAP_NPWD:
			return rootsim_config.checkpointing == STATE_SAVING_COPY;
		case CAP_FULL:
			return true;
		case CAP_INC:
			return false; // not implemented
		case CAP_A:
			return false; // not implemented
		case CAP_DETERMINISTIC_SEED:
			return rootsim_config.deterministic_seed;
		case CAP_SERIAL:
			return rootsim_config.serial;
		case CAP_CORE_BINDING:
			return !rootsim_config.core_binding;
		case CAP_PREEMPTION:
			return !rootsim_config.disable_preemption;
		case CAP_ECS:
			return false; // not implemented
		case CAP_LINUX_MODULES:
			return false; // not implemented
		case CAP_MPI:
			#ifdef HAVE_MPI
			return true;
			#else
			return false;
			#endif
		case CAP_SILENT:
			return rootsim_config.silent_output;
		default:
			break; // Just to silence compiler's warnings [-Wswitch]: other values are evaluated later
	}
	

	// With respect to the above, evaluate here in case no information is requested
	if(info == NULL) {
		switch(which) {
			case CAP_SCHEDULER ... CAP_VERBOSE:
				return true;
			case CAP_SIMULATION_TIME:
				return rootsim_config.simulation_time > 0;
			case CAP_POWER:
				return rootsim_config.powercap > 0.0;
			default:
				rootsim_error(false, "Requesting information for an unknown capability\n");
				return false;
		}
	}

	// Here we do the most work
	info->capability = which;

	switch(which) {
		case CAP_SCHEDULER:
			info->scheduler = rootsim_config.scheduler;
			return true;
		case CAP_CKTRM_MODE:
			info->termination_mode = rootsim_config.check_termination_mode;
			return true;
		case CAP_LPS_DISTRIBUTION:
			info->lps_distribution = rootsim_config.lps_distribution;
			return true;
		case CAP_STATS:
			info->stats = rootsim_config.stats;
			return true;
		case CAP_STATE_SAVING:
			info->state_saving = rootsim_config.ckpt_mode;
			return true;
		case CAP_THREADS:
			info->lps = n_cores;
			return true;
		case CAP_LPS:
			info->lps = n_prc_tot;
			return true;
		case CAP_OUTPUT_DIR:
			info->output_dir = rootsim_config.output_dir;
			return true;
		case CAP_P:
			info->ckpt_period = rootsim_config.ckpt_period;
			return true;
		case CAP_GVT:
			info->gvt_time_period = rootsim_config.gvt_time_period;
			return true;
		case CAP_SEED:
			info->seed = rootsim_config.set_seed;
			return true;
		case CAP_VERBOSE:
			info->verbose = rootsim_config.verbose;
			return true;
		case CAP_SIMULATION_TIME:
			info->simulation_time = rootsim_config.simulation_time;
			return rootsim_config.simulation_time > 0;
		case CAP_POWER:
			info->powercap = rootsim_config.powercap;
			return rootsim_config.powercap > 0.0;
		default:
			rootsim_error(false, "Requesting information for an unknown capability\n");
			return false;
	}
}
