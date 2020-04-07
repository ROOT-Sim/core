#include <lib/lib.h>

#include <lib/lib_internal.h>

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


void lib_lp_init(uint64_t llid)
{
	random_lib_lp_init(llid);
	state_lib_lp_init(llid);
}

void lib_lp_fini(void)
{

}
