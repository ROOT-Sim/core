#pragma once

#include <time.h>
#include <sys/time.h>

typedef int timer;

#define timer_new()							\
__extension__({								\
	struct timeval __tmp_timer;					\
	gettimeofday(&__tmp_timer, NULL);				\
	__tmp_timer.tv_sec * 1000 + __tmp_timer.tv_usec / 1000;		\
})

#define timer_value(ttimer)						\
__extension__({								\
	struct timeval __tmp_timer;					\
	gettimeofday(&__tmp_timer, NULL);				\
	__tmp_timer.tv_sec * 1000 + __tmp_timer.tv_usec / 1000 - ttimer;\
})
