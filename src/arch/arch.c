#include <arch/arch.h>

#if defined(__linux__)

#define _GNU_SOURCE
#include <core/core.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

static pthread_t *ptids;

void arch_signal_ignore(void)
{
	sigset_t mask, old_mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
}

bool arch_signal_want_end(void)
{
	struct timespec timeout = { .tv_sec = 0, .tv_nsec = 0 };
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);

	return sigtimedwait(&mask, NULL, &timeout) == SIGINT;
}

unsigned arch_core_count(void)
{
	long int ret = sysconf(_SC_NPROCESSORS_ONLN);
	return ret < 1 ? 1 : (unsigned)ret;
}

void arch_thread_create(
	unsigned t_cnt,
	bool affinity,
	void *(*t_fnc)(void *),
	void *t_fnc_arg
){
	ptids = mm_alloc(sizeof(*ptids) * t_cnt);

	pthread_attr_t t_attr;
	pthread_attr_init(&t_attr);

	while(t_cnt--){
		cpu_set_t c_set;

		CPU_ZERO(&c_set);
		CPU_SET(t_cnt, &c_set);

		if(affinity)
			pthread_attr_setaffinity_np(
				&t_attr, sizeof(c_set), &c_set);

		pthread_create(&ptids[t_cnt], &t_attr, t_fnc, t_fnc_arg);
	}

	pthread_attr_destroy(&t_attr);
}

void arch_thread_wait(unsigned t_cnt)
{
	while(t_cnt--){
		pthread_join(ptids[t_cnt], NULL);
	}

	mm_free(ptids);
}

#elif defined(_WIN64)

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>

unsigned arch_core_count(void)
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	return sys_info.dwNumberOfProcessors;
}

void arch_thread_init(
	unsigned thread_count,
	bool set_affinity,
	void *(*thread_fnc)(void *),
	void *thread_fnc_arg
){
	while (thread_count--) {
		HANDLE t_new = CreateThread(NULL, 0, thread_fnc, thread_fnc_arg,
			0, NULL);

		if(set_affinity)
			SetThreadAffinityMask(t_new, 1 << thread_count);
	}
}

#else /* OS_LINUX || OS_WINDOWS */
#error Unsupported operating system
#endif
