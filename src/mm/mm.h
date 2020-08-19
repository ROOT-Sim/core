#pragma once

#include <log/log.h>

#include <stdlib.h>
#include <stddef.h>

#ifdef NEUROME_SERIAL

#define __mm_alloc malloc
#define __mm_realloc realloc
#define __mm_free free

#else

extern void *__real_malloc(size_t mem_size);
extern void *__real_realloc(void *ptr, size_t mem_size);
extern void __real_free(void *ptr);

#define __mm_alloc __real_malloc
#define __mm_realloc __real_realloc
#define __mm_free __real_free

#endif

#ifdef NEUROME_TEST

#define mm_alloc malloc
#define mm_realloc realloc
#define mm_free free

#else

#pragma GCC poison malloc realloc free

inline void *mm_alloc(size_t mem_size)
{
	void *ret = __mm_alloc(mem_size);

	if (__builtin_expect(mem_size && !ret, 0)) {
		log_log(LOG_FATAL, "Out of memory!");
		abort();
	}
	return ret;
}

inline void *mm_realloc(void *ptr, size_t mem_size)
{
	void *ret = __mm_realloc(ptr, mem_size);

	if(__builtin_expect(mem_size && !ret, 0)) {
		log_log(LOG_FATAL, "Out of memory!");
		abort();
	}
	return ret;
}

inline void mm_free(void *ptr)
{
	__mm_free(ptr);
}

#endif
