/*  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#pragma once

#include <arch/thread.h>
#include <ROOT-Sim.h>
#include <stdbool.h>

#ifdef HAVE_CUDA

#if __cplusplus
 extern "C" {
#endif

extern bool gpu_is_available(void);
extern bool gpu_configure(lp_id_t n_lps);
extern thrd_ret_t THREAD_CALL_CONV gpu_main_loop(void *rid_arg);
extern void gpu_stop(void);

#if __cplusplus
 }
#endif

#else

#define gpu_is_available() (false)

#endif
