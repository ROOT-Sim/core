#include <test.h>

#ifndef NEUROME_TEST_LPS_COUNT
#define NEUROME_TEST_LPS_COUNT 64
#endif

#ifndef NEUROME_TEST_MODEL_NAME
#define NEUROME_TEST_MODEL_NAME "model"
#endif

static const char *test_arguments[] = {
	"--lp",
	NEUROME_TEST_LPS_COUNT,
	"--wt",
	"2",
	NULL
};

const struct _test_config_t test_config = {
	.test_name = "model " NEUROME_TEST_MODEL_NAME,
	.test_arguments = test_arguments,
};
