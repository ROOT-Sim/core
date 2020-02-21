#pragma once
#include <stddef.h>

typedef struct {


} mm_state;

extern void mm_alloc(size_t req_size);
extern void mm_free(void *ptr);
