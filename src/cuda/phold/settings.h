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

#define GVT_PERIOD 	(500*1000)
#define END_SIM_GVT  (30*1000*1000)
#define PHASE_WINDOW_SIZE 2000000

#define FAN_OUT 5

#define HOT_FRACTION 0.05
#define HOT_PHASE_PERIOD 2

#define NUM_LPS (8192*4)

#define CPU_ENABLED true
#define GPU_ENABLED false


#define	OPTM_SYNC	1
#define ALLOW_ME	1
