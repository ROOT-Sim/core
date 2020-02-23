#pragma once

#include <lib/lib.h>

#ifndef NEUROME_SERIAL
#include <lp/lp.h>
#else
#include <serial/serial.h>
#endif


#define lib_state (current_lp->lib_state)
