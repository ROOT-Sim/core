#include <lib/lib.h>

static char doc[] = "NeuRome model development libraries";
// this isn't needed (we haven't got non option arguments to document)
static char args_doc[] = "";

static const struct argp_option argp_options[] = {
	{0}
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	return ARGP_ERR_UNKNOWN;
}

const struct argp lib_argp = {argp_options, parse_opt, args_doc, doc, 0, 0, 0};
