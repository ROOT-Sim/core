#include <test.h>

#ifndef ROOTSIM_TEST_LPS_COUNT
#define ROOTSIM_TEST_LPS_COUNT 256
#endif

static const char *test_arguments[] = {
	"--lp",
	ROOTSIM_TEST_LPS_COUNT,
	"--wt",
	"2",
	NULL
};

const struct _test_config_t test_config = {
	.test_arguments = test_arguments,
};
