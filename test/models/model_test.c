#include <test.h>

#define ROOTSIM_TEST_LPS_COUNT "256"

static const char *test_arguments[] = {
	"--lp",
	ROOTSIM_TEST_LPS_COUNT,
	NULL
};

const struct test_config test_config = {
	.test_arguments = test_arguments,
};
