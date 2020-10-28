#include <lib/lib.h>

#include <lib/lib_internal.h>

static char lib_doc[] = "NeuRome model development libraries";
// this isn't needed (we haven't got non option arguments to document)
static char lib_args_doc[] = "";

static const struct argp_option lib_argp_options[] = {
	{0}
};

static error_t lib_parse_opt (int key, char *arg, struct argp_state *state)
{
	(void)key;
	(void)arg;
	(void)state;
	// TODO parsing options for model's library
	return ARGP_ERR_UNKNOWN;
}

const struct argp lib_argp = {lib_argp_options, lib_parse_opt, lib_args_doc, lib_doc, 0, 0, 0};

void lib_global_init(void)
{
	topology_global_init();
}

void lib_global_fini(void)
{

}

void lib_lp_init(void)
{
	random_lib_lp_init();
	state_lib_lp_init();
}

void lib_lp_fini(void)
{

}
