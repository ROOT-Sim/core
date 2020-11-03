/**
* @file core/timer.h
*
* @brief Timers
*
* This header defines the timers which the simulator uses to monitor its
* internal behaviour
*
* @copyright
* Copyright (C) 2008-2020 HPDCS Group
* https://hpdcs.github.io
*
* This file is part of ROOT-Sim (ROme OpTimistic Simulator).
*
* ROOT-Sim is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; only version 3 of the License applies.
*
* ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include <time.h>
#include <sys/time.h>

typedef uint_fast64_t timer;

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
