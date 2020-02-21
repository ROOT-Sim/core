#include <core/init.h>

#include <datatypes/bitmap.h>

#include <stdlib.h>
#include <limits.h>
#include <argp.h>
#include <unistd.h>
#include <inttypes.h>

/// Macro to get the core count on the hosting machine
#define get_cores() ((unsigned) sysconf(_SC_NPROCESSORS_ONLN))

simulation_configuration global_config;

/// This is the list of mnemonics for arguments
enum _opt_codes{
	OPT_FIRST = 128, /**< this is used as an offset to the enum values, so that argp doesn't assign short options */

	OPT_NPRC,
	OPT_LOG,
#ifndef NEUROME_SERIAL
	OPT_NP,
	OPT_GVT,
#endif
	OPT_LAST
};

const char *argp_program_version 	= "\nCopyright (C) 2020-2020 Andrea Piccione";
const char *argp_program_bug_address 	= "piccions@gmx.com";

// Directly from argp documentation:
// If non-zero, a string containing extra text to be printed before and after the options in a long help message,
// with the two sections separated by a vertical tab ('\v', '\013') character.
// By convention, the documentation before the options is just a short string explaining what the program does.
// Documentation printed after the options describe behavior in more detail.
static char doc[] = "NeuRome";

// this isn't needed (we haven't got non option arguments to document)
static char args_doc[] = "";
// the options recognized by argp
static const struct argp_option argp_options[] = {
	{"lp",			OPT_NPRC,		"VALUE",	0,		"Total number of Logical Processes being launched at simulation startup", 0},
	{"log-level",		OPT_LOG,		"TYPE",		0,		"Logging level", 0},
	{"verbose",		OPT_LOG,		"TYPE",		OPTION_ALIAS,	NULL, 0},
#ifndef NEUROME_SERIAL
	{"wt",			OPT_NP,			"VALUE",	0,		"Number of total cores being used by the simulation", 0},
	{"gvt",			OPT_GVT,		"VALUE",	0,		"Time between two GVT reductions (in milliseconds)", 0},
#endif
	{0}
};

static void print_config(){

}

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


#define parse_ullong_limits(low, high) 	\
	__extension__({										\
		unsigned long long int __value;							\
		char *__endptr;									\
		__value = strtoull(arg, &__endptr, 10);						\
		if(*arg == '\0' || *__endptr != '\0' || __value < low || __value > high) {	\
			malformed_option_failure();						\
		}										\
		__value;									\
	})

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	// this is used in order to ensure that the user doesn't use duplicate options
	static block_bitmap scanned[bitmap_required_size(OPT_LAST - OPT_FIRST)];

	if(key >= OPT_FIRST && key < OPT_LAST){
		if(bitmap_check(scanned, key - OPT_FIRST))
			conflicting_option_failure("this option has already been specified");

		bitmap_set(scanned, key - OPT_FIRST);
	}

	switch (key) {
		case OPT_NPRC:
			global_config.lps_cnt = parse_ullong_limits(1, UINT_MAX);
			break;

		case OPT_LOG:
			// TODO
			break;
#ifndef NEUROME_SERIAL
		case OPT_NP:
			if(strcmp(arg, "auto") == 0){
				global_config.threads_cnt = get_cores();
			}else{
				global_config.threads_cnt = parse_ullong_limits(1, UINT_MAX);
			}
			break;

		case OPT_GVT:
			global_config.gvt_period = parse_ullong_limits(1, 10000);
			break;
#endif
		case ARGP_KEY_INIT:
			memset(&global_config, 0, sizeof(global_config));
			memset(scanned, 0, sizeof(scanned));
			// Store the predefined values, before reading any overriding one
			// TODO
			break;

		case ARGP_KEY_SUCCESS:

			// sanity checks
			if(!bitmap_check(scanned, OPT_NPRC - OPT_FIRST))
				argp_error(state, "Number of LPs was not provided \"--lp\"\n");

#ifndef NEUROME_SERIAL
			if(!bitmap_check(scanned, OPT_NP - OPT_FIRST))
				argp_error(state, "Number of cores was not provided \"--wt\"\n");

			if(global_config.threads_cnt > get_cores())
				argp_error(state, "Demanding %u cores, which are more than available (%u)\n", global_config.threads_cnt, get_cores());

			if(global_config.lps_cnt < global_config.threads_cnt)
				argp_error(state, "Requested a simulation run with %" PRIu64 " LPs and %u worker threads: the mapping is not possible\n", global_config.lps_cnt, global_config.threads_cnt);
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
#undef conflicting_option_failure
#undef malformed_option_failure

static struct argp_child argp_child[2] = {
		{0, 0, "Model specific options", 0},
		{0}
};

static struct argp argp = {argp_options, parse_opt, args_doc, doc, argp_child, 0, 0};

void parse_args(int argc, char **argv)
{
	argp_child[0].argp = &model_argp;
	argp_parse (&argp, argc, argv, 0, NULL, NULL);
}

int parallel_simulation_start(void)
{
	return 0;
}

