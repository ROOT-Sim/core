#include <core/init.h>
#include <serial/serial.h>

int main(int argc, char **argv)
{
	log_log(LOG_INFO, "Started NeuROME");

	parse_args(argc, argv);

	int ret;

	if(global_config.serial){
		ret = serial_simulation_start();
	} else {
		ret = parallel_simulation_start();
	}

	return ret;
}
