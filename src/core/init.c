#include <core/init.h>

#include <arch/arch.h>
#include <core/core.h>
#include <lib/lib.h>

#include <argp.h>
#include <inttypes.h>
#include <limits.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

simulation_configuration global_config;

/// This is the list of mnemonics for arguments
enum _opt_codes{
	OPT_FIRST = 128, /**< we don't want argp short options */

	OPT_NPRC,
	OPT_LOG,
	OPT_CLOG,
	OPT_SIMT,
	OPT_GVT,
#ifndef NEUROME_SERIAL
	OPT_NP,
	OPT_BIND,
#endif
	OPT_LAST
};

const char *argp_program_version = "\nCopyright (C) 2020-2020 Andrea Piccione";
const char *argp_program_bug_address = "piccions@gmx.com";

// Directly from argp documentation:
// If non-zero, a string containing extra text to be printed before and after
// the options in a long help message, with the two sections separated by a
// vertical tab ('\v', '\013') character. By convention, the documentation
// before the options is just a short string explaining what the program does.
// Documentation printed after the options describe behavior in more detail.
static char doc[] = "NeuRome";

// this isn't needed (we haven't got non option arguments to document)
static char args_doc[] = "";
// the options recognized by argp
static const struct argp_option argp_options[] = {
	{"lp", 		OPT_NPRC, "VALUE", 	0, "Total number of Logical Processes being launched at simulation startup", 0},
	{"log-level", 	OPT_LOG, "TYPE",	0, "Logging level", 0},
	{"verbose", 	OPT_LOG, "TYPE",	OPTION_ALIAS, NULL, 0},
	{"time", 	OPT_SIMT, "VALUE",	0, "Logical time at which the simulation will be considered completed", 0},
	{"gvt-period", 	OPT_GVT, "VALUE",	0, "Time between two GVT reductions (in milliseconds)", 0},
#ifndef NEUROME_SERIAL
	{"wt", 		OPT_NP, "VALUE",	0, "Number of total cores being used by the simulation", 0},
	{"no-bind", 	OPT_BIND, NULL,		0, "Disables thread to core binding", 0},
#endif
	{0}
};

static void print_config(void)
{
	//TODO
}

#define malformed_option_failure() argp_error(state, "invalid value \"%s\" in %s option.\nAborting!", arg, state->argv[state->next -1 -(arg != NULL)])

#define parse_ullong_limits(low, high)					\
	__extension__({							\
		unsigned long long int __value;				\
		char *__endptr;						\
		__value = strtoull(arg, &__endptr, 10);			\
		if(							\
			*arg == '\0' 		||			\
			*__endptr != '\0' 	||			\
			__value < low 		||			\
			__value > high) {				\
			malformed_option_failure();			\
		}							\
		__value;						\
	})

#define parse_ldouble_limits(low, high)					\
	__extension__({							\
		long double __value;					\
		char *__endptr;						\
		__value = strtold(arg, &__endptr);			\
		if(							\
			*arg == '\0' 		||			\
			*__endptr != '\0' 	||			\
			__value < low 		||			\
			__value > high) {				\
			malformed_option_failure();			\
		}							\
		__value;						\
	})

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	switch (key) {

	case OPT_NPRC:
		n_lps = parse_ullong_limits(1, UINT_MAX);
		break;

	case OPT_LOG:
		// TODO
		break;

	case OPT_SIMT:
		global_config.termination_time = parse_ldouble_limits(0,
			SIMTIME_MAX);
		break;

	case OPT_GVT:
		global_config.gvt_period = parse_ullong_limits(1, 10000) * 1000;
		break;

#ifndef NEUROME_SERIAL
	case OPT_NP:
		n_threads = parse_ullong_limits(1, UINT_MAX);
		break;

	case OPT_BIND:
		global_config.core_binding = false;
		break;
#endif

	case ARGP_KEY_INIT:
		memset(&global_config, 0, sizeof(global_config));
#ifndef NEUROME_SERIAL
		n_threads = arch_core_count();
		global_config.core_binding = true;
#endif
		global_config.gvt_period = 200000;
		global_config.termination_time = SIMTIME_MAX;
		log_colored = isatty(STDERR_FILENO);
		// Store the predefined values, before reading any overriding one
		// TODO
		break;

	case ARGP_KEY_SUCCESS:

		if(n_lps == 0)
			argp_error(
				state,
				"Number of LPs was not provided \"--lp\"\n"
			);

#ifndef NEUROME_SERIAL
		if(n_threads > arch_core_count())
			argp_error(
				state,
				"Demanding %u cores, which are more than available (%u)\n",
				n_threads,
				arch_core_count()
			);
#endif

		log_logo_print();
		print_config();
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

#undef parse_ullong_limits
#undef handle_string_option
#undef malformed_option_failure

static struct argp_child argp_child[] = {
		{&lib_argp, 0, "Model library options", 0},
		{&model_argp, 0, "Model specific options", 0},
		{0}
};

static const struct argp argp = {
	argp_options, parse_opt, args_doc, doc, argp_child, 0, 0};

void init_args_parse(int argc, char **argv)
{
	argp_parse (&argp, argc, argv, 0, NULL, NULL);
}
