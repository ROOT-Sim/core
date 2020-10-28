<<<<<<< HEAD
/*
 * heap.h
 *
 *  Created on: 30 giu 2018
 *      Author: andrea
 */

#ifndef __HEAP_H_
#define __HEAP_H_

#include <datatypes/array.h>


#define rootsim_heap(type) rootsim_array(type)

#define heap_init(self)	array_init(self)

#define heap_empty(self) array_empty(self)

#define heap_insert(self, elem, cmp_f) ({\
		__typeof(array_count(self)) __i_h = array_count(self);\
		array_push(self, elem);\
		while(__i_h && cmp_f(elem, array_items(self)[(__i_h - 1)/2]) < 0){\
			array_items(self)[__i_h] = array_items(self)[(__i_h - 1)/2];\
			__i_h = (__i_h - 1)/2;\
		}\
		array_items(self)[__i_h] = elem;\
	})

#define heap_min(self) ((const)(array_items(self)[0]))

#define heap_extract(self, cmp_f) ({\
		__typeof(*array_items(self)) __ret_h = array_items(self)[0]; \
		__typeof(*array_items(self)) __last_h = array_pop(self); \
		__typeof(array_count(self)) __i_h = 0; \
		do{ \
			__i_h <<= 1; \
			++__i_h; \
			if(__i_h >= array_count(self)){\
				break; \
			} \
			if(__i_h + 1 < array_count(self)){ \
				if(cmp_f(array_items(self)[__i_h + 1], array_items(self)[__i_h]) < 0) {\
					++__i_h;\
				} \
			}\
			if(cmp_f(array_items(self)[__i_h], __last_h) < 0) {\
				array_items(self)[(__i_h-1) >> 1] = array_items(self)[__i_h]; \
			} \
			else{ \
				break; \
			}\
		} while(1); \
		array_items(self)[(__i_h-1) >> 1] = __last_h; \
		__ret_h; \
	})

#endif /* __HEAP_H_ */

/*typedef struct {
    double priority;
    void *data;
} node_heap_t;

typedef struct {
    node_heap_t *nodes;
    int len;
    int size;
} heap_t;*/
=======
/**
*			Copyright (C) 2008-2018 HPDCS Group
*			http://www.dis.uniroma1.it/~hpdcs
*
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
*
* @file heap.h
* @brief This header defines macros for accessing the heap
* 	 implementation used in the simulator.
* @author Stefano Conoci
* @date November 2, 2018
*/

#pragma once 
 
typedef struct {
       	double priority;
	void *data;
} node_heap_t;
 
typedef struct {
	node_heap_t *nodes;
	int len;
	int size;
} heap_t;

void heap_push(heap_t *, double, void *);

// Returns data sorted from lowest to highest priority
void* heap_pop(heap_t *);
>>>>>>> origin/power
