#include <core/core.h>

#include <core/init.h>
#include <distributed/mpi.h>
#include <parallel/parallel.h>
#include <serial/serial.h>

int main(int argc, char **argv)
{
#ifdef ROOTSIM_MPI
	mpi_global_init(&argc, &argv);
#endif
	init_args_parse(argc, argv);

	if (global_config.is_serial) {
		serial_simulation();
	} else {
		parallel_simulation();
	}

#ifdef ROOTSIM_MPI
	mpi_global_fini();
#endif
}
