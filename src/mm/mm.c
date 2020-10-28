#include <mm/mm.h>

extern void *mm_alloc(size_t mem_size);
extern void *mm_realloc(void *ptr, size_t mem_size);
extern void mm_free(void *ptr);
