#pragma once

#include <stdbool.h>

extern void arch_signal_ignore(void);
extern void arch_signal_sigint_setup(void (handler)(int));
extern unsigned arch_core_count(void);
extern void arch_thread_create(unsigned t_cnt, bool affinity,
	void *(*t_fnc)(void *), void *t_fnc_arg);
extern void arch_thread_wait(unsigned t_cnt);
