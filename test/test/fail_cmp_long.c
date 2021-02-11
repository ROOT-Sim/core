#include <test.h>

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	test_printf("aa");
	return 0;
}

const struct _test_config_t test_config = {
	.expected_output_size = 1,
	.expected_output = "ab"
};
