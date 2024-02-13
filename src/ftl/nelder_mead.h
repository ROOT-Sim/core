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

#ifndef nelder_mead_h
#define nelder_mead_h

#include <stdio.h>
#include <float.h>

void nm_start(int start, double start_x, double start_y, double start_f);
void nm_get_next_point(double last_f, double *next_x, double *next_y);
void nm_init(double (*func_)(double, double, int, double*, int), double max_wall_step_, double *dp_);
void nm_optimize(double *x, double *y);

#endif
