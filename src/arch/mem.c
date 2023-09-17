/**
 * @file arch/mem.c
 *
 * @brief Platform specific memory utilities
 *
 * This module implements some memory related utilities such as memory
 * statistics retrieval in a platform independent way
 *
 * SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
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

/**
 * @fn mem_deterministic_alloc(void *ptr, size_t mem_size)
 * @brief Tries to allocate a memory region at a given address
 * @param ptr where the allocation has to start, must be aligned to at least 2MiB
 * @param mem_size the size in bytes of the requested allocation
 * @return 0 if successful, -1 otherwise
 */

/**
 * @fn mem_deterministic_free(void *ptr, size_t mem_size)
 * @brief Frees a memory region allocated with mem_deterministic_alloc()
 */

#ifdef __POSIX

#include <sys/mman.h>
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

int mem_deterministic_alloc(void *ptr, size_t mem_size)
{
	int flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED; // FIXME: may unmap already mapped memory to satisfy the call
	return -(mmap(ptr, mem_size, PROT_READ | PROT_WRITE, flags, 242, 0) == (void *)-1);
}

#elif defined(__LINUX)

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static int statm_fd;
static long linux_page_size;

int mem_stat_setup(void)
{
	/* Flawfinder: ignore */
	statm_fd = open("/proc/self/statm", O_RDONLY);
	if(statm_fd == -1)
		return -1;

	linux_page_size = sysconf(_SC_PAGESIZE);
	return 0;
}

size_t mem_stat_rss_current_get(void)
{
	/* Flawfinder: ignore */
	char res[40]; // sufficient for two 64 bit base 10 numbers and a space
	/* Flawfinder: ignore */
	if(__builtin_expect(lseek(statm_fd, 0, SEEK_SET) == -1 || read(statm_fd, res, sizeof(res) - 1) == -1, 0))
		return (size_t)0;

	res[sizeof(res) - 1] = '\0';

	size_t i = 0;
	while(res[i] && !isspace(res[i]))
		++i;

	return (size_t)(strtoull(&res[i], NULL, 10) * linux_page_size);
}

int mem_deterministic_alloc(void *ptr, size_t mem_size)
{
	int flags = (21 << MAP_HUGE_SHIFT) | // 2 MiB huge pages
	            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_HUGETLB | MAP_FIXED_NOREPLACE;
	return -(mmap(ptr, mem_size, PROT_READ | PROT_WRITE, flags, -1, 0) == MAP_FAILED);
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

void mem_deterministic_free(void *ptr, size_t mem_size)
{
	munmap(ptr, mem_size);
}

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

int mem_deterministic_alloc(void *ptr, size_t mem_size)
{
	// TODO use huge pages (honestly, Win APIs are such a mess)
	return -(VirtualAlloc(ptr, mem_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) == NULL);
}

void mem_deterministic_free(void *ptr, size_t mem_size)
{
	VirtualFree(ptr, 0, MEM_RELEASE);
}

#endif
