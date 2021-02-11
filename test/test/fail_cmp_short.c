#include <test.h>

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	test_printf("a");
	return 0;
}

const struct _test_config_t test_config = {
	.expected_output_size = 2,
	.expected_output = "aa"
};
