#pragma once

extern unsigned arch_core_count		(void);
extern void 	arch_thread_init	(unsigned thread_count, void *(*thread_fnc)(void *), void *thread_fnc_arg);
extern void 	arch_affinity_set	(unsigned thread_id);
