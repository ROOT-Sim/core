#pragma once

#include <log/log.h>

#include <stdlib.h>
#include <stddef.h>

extern void *__real_malloc(size_t mem_size);
extern void *__real_realloc(void *ptr, size_t mem_size);
extern void __real_free(void *ptr);

#define __mm_alloc malloc
#define __mm_realloc realloc
#define __mm_free free

#if defined(NEUROME_TEST)

#define mm_alloc(mem_size) malloc(mem_size);

#define mm_realloc(ptr, mem_size) realloc(ptr, mem_size)

#define mm_free(ptr) free(ptr)

#elif defined(NEUROME_SERIAL)

#define mm_alloc(mem_size) 				\
__extension__({						\
	size_t __m_size = mem_size;			\
	void *__ret = __mm_alloc(__m_size);		\
	if(unlikely((__m_size) != 0 && !__ret)){	\
		log_log(LOG_FATAL, "Out of memory!");	\
		abort();				\
	}						\
	__ret;						\
})

#define mm_realloc(ptr, mem_size) 			\
__extension__({						\
	size_t __m_size = mem_size;			\
	void *__ret = __mm_realloc(ptr, __m_size);	\
	if(unlikely((__m_size) != 0 && !__ret)){	\
		log_log(LOG_FATAL, "Out of memory!");	\
		abort();				\
	}						\
	__ret;						\
})

#define mm_free(ptr) __mm_free(ptr)

#pragma GCC poison malloc realloc free

#else

// todo improve the abortion mechanism or even try to recover
#define mm_alloc(mem_size) 				\
__extension__({						\
	size_t __m_size = mem_size;			\
	void *__ret = __real_malloc(__m_size);		\
	if (unlikely((__m_size) != 0 && !__ret)){	\
		log_log(LOG_FATAL, "Out of memory!");	\
		abort();				\
	}						\
	__ret;						\
})

#define mm_realloc(ptr, mem_size) 			\
__extension__({						\
	size_t __m_size = mem_size;			\
	void *__ret = __real_realloc(ptr, __m_size);	\
	if (unlikely((__m_size) != 0 && !__ret)){	\
		log_log(LOG_FATAL, "Out of memory!");	\
		abort();				\
	}						\
	__ret;						\
})

#define mm_free(ptr) __real_free(ptr)

#pragma GCC poison malloc realloc free

#endif
