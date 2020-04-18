#include <test.h>

#ifndef LPS_COUNT
#define LPS_COUNT 64
#endif

#ifndef MODEL_NAME
#define MODEL_NAME "model"
#endif

static const char *test_arguments[] = {
	"--lp",
	LPS_COUNT,
	"--wt",
	"2",
	NULL
};

const struct _test_config_t test_config = {
	.test_name = "model " MODEL_NAME,
	.test_arguments = test_arguments,
};
