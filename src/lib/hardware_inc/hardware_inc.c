#include <lib/hardware_inc/hardware_inc.h>

#include <core/init.h>
#include <log/log.h>
#include <mm/buddy/buddy.h>

#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>

/* Use 'j' as magic number */
#define PTWRITE_IOC_MAGIC		'j'

#define _IO_NB	1

/* No parameters */
#define PTWRITE_PMC_SETUP	_IO(PTWRITE_IOC_MAGIC, _IO_NB)
#define PTWRITE_PROFILER_ON	_IO(PTWRITE_IOC_MAGIC, _IO_NB + 1)
#define PTWRITE_PROFILER_OFF	_IO(PTWRITE_IOC_MAGIC, _IO_NB + 2)
/* Read from kernel space */
#define PTWRITE_GET_PAYLOADS	_IOR(PTWRITE_IOC_MAGIC, _IO_NB + 3, void *)

#define HINC_DEV_NAME "/dev/ptwrite"
#define MEM_SIZE (4096 << 3)

static int hinc_fd;
static _Thread_local uint64_t payloads[MEM_SIZE];

void hardware_inc_global_init(void)
{
	if (!global_config.hinc)
		return;

	hinc_fd = open(HINC_DEV_NAME, 0666);
	if (hinc_fd < 0) {
		log_log(LOG_FATAL, "Error, cannot open %s", HINC_DEV_NAME);
		exit(-1);
	}
}

void hardware_inc_init(void)
{
	if (ioctl(hinc_fd, PTWRITE_PROFILER_ON) < 0){
		log_log(LOG_FATAL, "IOCTL: PTWRITE_PROFILER_ON failed");
		exit(-1);
	}
}

void hardware_inc_fini(void)
{
	if (ioctl(hinc_fd, PTWRITE_PROFILER_OFF) < 0)
		log_log(LOG_ERROR, "IOCTL: PTWRITE_PROFILER_OFF failed");
}

void hardware_inc_global_fini(void)
{
	if (!global_config.hinc)
		return;

	if (close(hinc_fd) < 0)
		log_log(LOG_ERROR, "Error, cannot close %s", HINC_DEV_NAME);
}

void hardware_inc_on_take(void)
{
	if (!global_config.hinc)
		return;

	long i = ioctl(hinc_fd, PTWRITE_GET_PAYLOADS, payloads);
	if (!i) {
		log_log(LOG_ERROR, "IOCTL: PTWRITE_GET_PAYLOADS failed\n");
		return;
	}

	while (i--) {
		uint64_t p = payloads[i];
		__write_mem((void *)(p & ((1ULL << 48ULL) - 1)), p >> 48);
	}

}

void hardware_inc_on_restore(void)
{
	if (!global_config.hinc)
		return;

	// TODO ask Stefano for an IOCTL to clear the internal kernel buffer
	ioctl(hinc_fd, PTWRITE_GET_PAYLOADS, payloads);
}

void __write_mem_gen(const void *ptr, size_t s)
{
	if (!global_config.hinc)
		__write_mem(ptr, s);
	else {
		uintptr_t v = ((uintptr_t)ptr) & ((1ULL << 48ULL) - 1);
		__builtin_ia32_ptwrite64(v | (s << 48ULL));
	}
}
