#pragma once

#include <lib/lib.h>

#ifdef NEUROME_SERIAL
#include <serial/serial.h>
#define l_s_p			(&cur_lp->ls)
#define l_s_m_p 		(&cur_lp->lsm)
#define current_lid		(cur_lp - lps)
#else
#include <lp/lp.h>
#define l_s_p			(&current_lp->ls)
#define l_s_m_p 		(current_lp->lsm_p)
#define current_lid		(current_lp - lps)
#endif

#ifdef NEUROME_INCREMENTAL
#include <mm/model_allocator.h>
#define mark_written(ptr, size)	__write_mem(ptr, size)
#else
#define mark_written(ptr, size)
#endif
