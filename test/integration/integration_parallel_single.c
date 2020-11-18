#include <test.h>

#include <integration/model/output.h>

#include <stdlib.h>

static const char *test_arguments[] = {
	"--lp",
	"64",
	"--wt",
	"1",
	NULL
};

const struct _test_config_t test_config = {
	.test_name = "integration parallel single",
	.test_arguments = test_arguments,
	.expected_output = expected_output,
	.expected_output_size = sizeof(expected_output)
};