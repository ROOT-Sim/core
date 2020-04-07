#pragma once

#include <time.h>
#include <sys/time.h>

typedef unsigned timer;

#define timer_new()							\
__extension__({								\
	struct timeval __tmp_timer;					\
	gettimeofday(&__tmp_timer, NULL);				\
	__tmp_timer.tv_sec * 1000000 + __tmp_timer.tv_usec;		\
})

#define timer_value(ttimer)						\
__extension__({								\
	struct timeval __tmp_timer;					\
	gettimeofday(&__tmp_timer, NULL);				\
	__tmp_timer.tv_sec * 1000000 + __tmp_timer.tv_usec - ttimer;\
})
