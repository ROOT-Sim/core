#include <test.h>

// FIXME this test is totally dummy and useless

static const char *test_arguments[] = {
	"--version",
	NULL
};

const struct _test_config_t test_config = {
	.test_name = "compiler",
	.test_arguments = test_arguments
};
