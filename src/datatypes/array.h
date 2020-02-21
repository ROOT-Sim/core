#pragma once

#include <memory.h>
#include <stdlib.h>

#define arr_realloc realloc
#define arr_alloc malloc
#define arr_free free

#define INIT_SIZE_ARRAY 8U

#define dyn_array(type)					\
		struct {				\
			type *items;			\
			unsigned count, capacity;	\
		}

// you can use the array to directly index items, but do at your risk and peril
#define array_items(self) ((self).items)

#define array_count(self) ((self).count)

#define array_capacity(self) ((self).capacity)
//this isn't checked CARE!
#define array_peek(self) (array_items(self)[array_count(self) - 1])
//this isn't checked CARE!
#define array_get_at(self, i) (array_items(self)[i])

#define array_is_empty(self) (array_count(self) == 0)

#define array_new(type)						\
	__extension__({						\
		dyn_array(type) *__newarr;			\
		__newarr = arr_alloc(sizeof(*__newarr));	\
		array_init(*__newarr);				\
		__newarr;					\
	})

#define array_free(self)					\
	__extension__({						\
		arr_free(array_items(self));			\
		arr_free(&(self));				\
	})

#define array_init(self)					\
	__extension__({						\
		array_capacity(self) = INIT_SIZE_ARRAY;		\
		array_items(self) =				\
			arr_alloc(array_capacity(self) *	\
			sizeof(*array_items(self))		\
		);						\
		array_count(self) = 0;				\
	})

#define array_fini(self)					\
	__extension__({						\
		arr_free(array_items(self));			\
	})

#define array_push(self, elem)					\
	__extension__({						\
		array_expand(self);				\
		array_items(self)[array_count(self)] = (elem);	\
		array_count(self)++;				\
	})

#define array_pop(self)						\
	__extension__({						\
		__typeof__(*array_items(self)) __popval;	\
		array_count(self)--;				\
		__popval = array_items(self)[array_count(self)];\
		array_shrink(self);				\
		__popval;					\
	})

#define array_add_at(self, i, elem)				\
	__extension__({						\
		array_expand(self);				\
		memmove(					\
			&(array_items(self)[(i)+1]),		\
			&(array_items(self)[(i)]),		\
			sizeof(*array_items(self)) *		\
				(array_count(self)-(i)		\
			)					\
		);						\
		array_items(self)[(i)] = (elem);		\
		array_count(self)++;				\
	})

#define array_lazy_remove_at(self, i)				\
	__extension__({						\
		__typeof__(*array_items(self)) __rmval;		\
		array_count(self)--;				\
		__rmval = array_items(self)[(i)];		\
		array_items(self)[(i)] =			\
			array_items(self)[array_count(self)];	\
		array_shrink(self);				\
		__rmval;					\
	})

#define array_remove_at(self, i)				\
	__extension__({						\
		__typeof__(*array_items(self)) __rmval;		\
		array_count(self)--;				\
		__rmval = array_items(self)[(i)];		\
		memmove(					\
			&(array_items(self)[(i)]),		\
			&(array_items(self)[(i)+1]),		\
			sizeof(*array_items(self)) *		\
				(array_count(self)-(i)		\
			)					\
		);						\
		array_shrink(self);				\
		__rmval;					\
	})

#define array_remove(self, elem)				\
	__extension__({						\
		typeof(array_count(self)) __cntr = 		\
			array_count(self);			\
		while(__cntr--){				\
			if(array_items(self)[__cntr] == (elem)){\
				array_remove_at(self, __cntr);	\
				break;				\
			}					\
		}						\
	})

#define array_dump_size(self)					\
	__extension__({						\
		sizeof(array_count(self)) +			\
		array_count(self) * sizeof(*array_items(self));	\
	})

#define array_dump(self, mem_area) \
	__extension__({ \
		memcpy((mem_area), &array_count(self), sizeof(array_count(self))); \
		(mem_area) = ((unsigned char *)(mem_area)) + sizeof(array_count(self)); \
		memcpy((mem_area), array_items(self), array_count(self) * sizeof(*array_items(self))); \
		mem_area = ((unsigned char *)(mem_area)) + array_count(self) * sizeof(*array_items(self)); \
	})

#define array_load(self, mem_area) \
	__extension__({ \
		memcpy(&array_count(self), (mem_area), sizeof(array_count(self))); \
		(mem_area) = ((unsigned char *)(mem_area)) + sizeof(array_count(self)); \
		array_capacity(self) = max(array_count(self), INIT_SIZE_ARRAY); \
		array_items(self) = rsalloc(array_capacity(self) * sizeof(*array_items(self))); \
		memcpy(array_items(self), (mem_area), array_count(self) * sizeof(*array_items(self))); \
		(mem_area) = ((unsigned char *)(mem_area)) + (array_count(self) * sizeof(*array_items(self))); \
	})

#define array_shrink(self) \
	__extension__({ \
		if (unlikely(array_count(self) > INIT_SIZE_ARRAY && array_count(self) * 3 <= array_capacity(self))) { \
			array_capacity(self) /= 2; \
			array_items(self) = arr_realloc(array_items(self), array_capacity(self) * sizeof(*array_items(self))); \
		} \
	})

#define array_expand(self) \
	__extension__({ \
		if(unlikely(array_count(self) >= array_capacity(self))){\
			array_capacity(self) *= 2; \
			array_items(self) = arr_realloc(array_items(self), array_capacity(self) * sizeof(*array_items(self))); \
		} \
	})
