#include <test.h>

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	fprintf(test_output_file, "aa");
	return 0;
}

const struct _test_config_t test_config = {
	.test_name = "fail cmp len",
	.expected_output_size = 2,
	.expected_output = "ab"
};
