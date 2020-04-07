#pragma once

#include <lib/lib.h>

#ifndef NEUROME_SERIAL
#include <lp/lp.h>
#define lib_state 		(current_lp->ls)
#define lib_state_managed 	(current_lp->lsm)
#else
#include <serial/serial.h>
#define lib_state 		(cur_lp->ls)
#define lib_state_managed 	(cur_lp->lsm)
#endif



