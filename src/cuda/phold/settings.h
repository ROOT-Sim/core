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

#define EVENT_HEADER	"phold/event.h"
#define STATE_HEADER	"phold/state.h"
#define MODEL_HEADER	"phold/model.h"

#define GVT_PERIOD 	(200*1000)
#define END_SIM_GVT  (26*1000*1000)
#define PHASE_WINDOW_SIZE (8*1000*1000)
#define HOT_FRACTION 0.01
#define HOT_PHASE_PERIOD 2

#define ENABLE_HOT  1

#define CPU_ENABLED true
#define GPU_ENABLED false

//#define NUM_THREADS 19
#define NUM_LPS (1024 * 38)

#define	OPTM_SYNC	1
#define ALLOW_ME	1
