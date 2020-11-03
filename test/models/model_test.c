#include <test.h>

#ifndef ROOTSIM_TEST_LPS_COUNT
#define ROOTSIM_TEST_LPS_COUNT 64
#endif

#ifndef ROOTSIM_TEST_MODEL_NAME
#define ROOTSIM_TEST_MODEL_NAME "model"
#endif

static const char *test_arguments[] = {
	"--lp",
	ROOTSIM_TEST_LPS_COUNT,
	"--wt",
	"2",
	NULL
};

const struct _test_config_t test_config = {
	.test_name = "model " ROOTSIM_TEST_MODEL_NAME,
	.test_arguments = test_arguments,
};
