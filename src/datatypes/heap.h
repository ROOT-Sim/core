#pragma once

#include <datatypes/array.h>

#define binary_heap(type) dyn_array(type)

#define heap_init(self)	array_init(self)
#define heap_items(self) array_items(self)
#define heap_count(self) array_count(self)
#define heap_fini(self)	array_fini(self)
#define heap_is_empty(self) array_is_empty(self)
#define heap_min(self) ((__typeof(array_items(self)[0]) const)(array_items(self)[0]))

#define heap_insert(self, cmp_f, elem)					\
	__extension__({							\
		__typeof(array_count(self)) __i_h = array_count(self);	\
		array_push(self, elem);					\
		while(							\
			__i_h && 					\
			cmp_f(elem, array_items(self)[(__i_h - 1)/2])	\
		){							\
			array_items(self)[__i_h] =			\
				array_items(self)[(__i_h - 1)/2];	\
			__i_h = (__i_h - 1)/2;				\
		}							\
		array_items(self)[__i_h] = elem;			\
	})

#define heap_extract(self, cmp_f)					\
	__extension__({							\
		__typeof(*array_items(self)) __ret_h = 			\
			array_items(self)[0];				\
		__typeof(*array_items(self)) __last_h = array_pop(self);\
		__typeof(array_count(self)) __i_h = 0;			\
		do{							\
			__i_h <<= 1;					\
			++__i_h;					\
			if(__i_h >= array_count(self)){			\
				break;					\
			}						\
			__i_h += __i_h + 1 < array_count(self) &&	\
				cmp_f(					\
					array_items(self)[__i_h + 1],	\
					array_items(self)[__i_h]	\
				);					\
			if(cmp_f(array_items(self)[__i_h], __last_h)){	\
				array_items(self)[(__i_h - 1) >> 1] =	\
					array_items(self)[__i_h];	\
			} else {					\
				break;					\
			}						\
		} while(1);						\
		array_items(self)[(__i_h - 1) >> 1] = __last_h;		\
		__ret_h;						\
	})
