#include <arch/arch.h>

#if defined(__linux__)

#include <sched.h>
#include <unistd.h>
#include <pthread.h>

unsigned arch_core_count(void)
{
	long int ret = sysconf(_SC_NPROCESSORS_ONLN);
	return ret < 1 ? 1 : (unsigned)ret;
}

void arch_thread_init(
	unsigned thread_count,
	void *(*thread_fnc)(void *),
	void *thread_fnc_arg
){
	pthread_t thread_id;
	while(thread_count--){
		pthread_create(&thread_id, NULL, thread_fnc, thread_fnc_arg);
	}
}

void arch_affinity_set(unsigned thread_id)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(thread_id, &cpuset);
	// 0 is the current thread
	sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

#elif defined(_WIN64)

#include <windows.h>

unsigned arch_core_count(void)
{
	SYSTEM_INFO _sysinfo;
	GetSystemInfo(&_sysinfo);
	return _sysinfo.dwNumberOfProcessors;
}

void arch_thread_init(
	unsigned thread_count,
	void *(*thread_fnc)(void *),
	void *thread_fnc_arg
){
	DWORD thread_id;
	while (thread_count--) {
		CreateThread(
			NULL,
			0,
			thread_fnc,
			thread_fnc_arg,
			0,
			&thread_id
		);
	}
}

void arch_affinity_set(unsigned thread_id)
{
	SetThreadAffinityMask(GetCurrentThread(), 1 << thread_id); // TODO handle processor groups bulls**t
}

#else /* OS_LINUX || OS_WINDOWS */
#error Unsupported operating system
#endif
