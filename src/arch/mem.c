/**
 * @file arch/mem.c
 *
 * @brief Platform specific memory utilities
 *
 * This module implements some memory related utilities such as memory
 * statistics retrieval in a platform independent way
 *
 * SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <arch/mem.h>

/**
 * @fn mem_stat_setup(void)
 * @brief Initialize the platform specific memory statistics facilities
 * @return 0 if successful, -1 otherwise
 */

/**
 * @fn mem_stat_rss_current_get(void)
 * @brief Get the current size of the resident set
 * @return the size in bytes of the current resident set, 0 if unsuccessful
 */

/**
 * @fn mem_stat_rss_max_get(void)
 * @brief Get the maximum size of the resident set since program beginning
 * @return the size in bytes of the maximal resident set, 0 if unsuccessful
 */

#ifdef __POSIX

#include <sys/resource.h>

#if defined(__MACOS)

#include <mach/mach_init.h>
#include <mach/mach_port.h>
#include <mach/task.h>
#include <mach/task_info.h>

int mem_stat_setup(void)
{
	return 0;
}

size_t mem_stat_rss_current_get(void)
{
	struct mach_task_basic_info info;
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
	if(__builtin_expect(task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS, 0))
		return (size_t)0;
	return (size_t)info.resident_size;
}

#elif defined(__LINUX)

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static int proc_stat_fd;
static long linux_page_size;

int mem_stat_setup(void)
{
	/* Flawfinder: ignore */
	proc_stat_fd = open("/proc/self/statm", O_RDONLY);
	if(proc_stat_fd == -1)
		return -1;

	linux_page_size = sysconf(_SC_PAGESIZE);
	return 0;
}

size_t mem_stat_rss_current_get(void)
{
	/* Flawfinder: ignore */
	char res[40]; // sufficient for two 64 bit base 10 numbers and a space
	/* Flawfinder: ignore */
	if(__builtin_expect(lseek(proc_stat_fd, 0, SEEK_SET) == -1 || read(proc_stat_fd, res, sizeof(res) - 1) == -1, 0))
		return (size_t)0;

	res[sizeof(res) - 1] = '\0';

	size_t i = 0;
	while(res[i] && !isspace(res[i])) {
		++i;
	}

	return (size_t)(strtoull(&res[i], NULL, 10) * linux_page_size);
}

#else

int mem_stat_setup(void)
{
	return 0;
}

size_t mem_stat_rss_current_get(void)
{
	return 0;
}

#endif

size_t mem_stat_rss_max_get(void)
{
	struct rusage res;
	getrusage(RUSAGE_SELF, &res);

#ifdef __MACOS
	return (size_t)res.ru_maxrss;
#else
	return (size_t)res.ru_maxrss * 1024;
#endif
}

#endif

#ifdef __WINDOWS

#include <windows.h>
#include <psapi.h>

int mem_stat_setup(void)
{
	return 0;
}

size_t mem_stat_rss_current_get(void)
{
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return (size_t)info.WorkingSetSize;
}

size_t mem_stat_rss_max_get(void)
{
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return (size_t)info.PeakWorkingSetSize;
}

#endif
