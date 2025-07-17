/**
* @file datatypes/map.h
*
* @brief Hash map datatype
*
* Dynamic array datatype
*
* SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <datatypes/array.h>

#include <assert.h>

#define MAP_INITIAL_CAPACITY 8
#define MAP_MAX_LOAD_XA 112
#define MAP_MAX_LOAD_XB 128

_Static_assert(MAP_MAX_LOAD_XA < MAP_MAX_LOAD_XB, "Incorrect map max load factor numbers");

// TODO check effect of casts before/after bitwise operations (key, cmo, dib, ...)

#define map_declare(pair_type)                                                                                         \
    struct {                                                                                                           \
        pair_type *pairs;                                                                                              \
        array_count_t count;                                                                                           \
        array_count_t capacity_mo;                                                                                     \
    }

#define map_capacity_mo(self) ((self).capacity_mo)
#define map_count(self) ((self).count)
#define map_pairs(self) ((self).pairs)

#define map_init(self)                                                                                                 \
    __extension__({                                                                                                    \
        map_count(self) = 0;                                                                                           \
        map_capacity_mo(self) = MAP_INITIAL_CAPACITY - 1;                                                              \
        map_pairs(self) = mm_alloc(MAP_INITIAL_CAPACITY * sizeof(*map_pairs(self)));                                   \
        memset(map_pairs(self), 0, sizeof(*map_pairs(self)) * MAP_INITIAL_CAPACITY);                                   \
    })

#define map_fini(self) __extension__({ mm_free(map_pairs(self)); })

#define map_insert_internal(self, pair_key_get, pair_is_empty, elem)                                                   \
    __extension__({                                                                                                    \
        array_count_t cmo = map_capacity_mo(self), dib = 0, l = (array_count_t)(pair_key_get(elem) & cmo);             \
        __typeof__(*map_pairs(self)) *pairs = map_pairs(self), pt = elem;                                              \
        while(!pair_is_empty(pairs[l])) {                                                                              \
            array_count_t this_dib = (array_count_t)(l - pair_key_get(pairs[l])) & cmo;                                \
            if(this_dib < dib) {                                                                                       \
                dib = this_dib;                                                                                        \
                __typeof__(*map_pairs(self)) tmp = pt;                                                                 \
                pt = pairs[l];                                                                                         \
                pairs[l] = tmp;                                                                                        \
            }                                                                                                          \
            dib++;                                                                                                     \
            l = (l + 1) & cmo;                                                                                         \
        }                                                                                                              \
        pairs[l] = pt;                                                                                                 \
    })

#define map_expand(self, pair_key_get, pair_is_empty)                                                                  \
    __extension__({                                                                                                    \
        assert(map_capacity_mo(self) < ARRAY_COUNT_MAX / 2);                                                           \
        map_capacity_mo(self) = 2 * map_capacity_mo(self) + 1;                                                         \
                                                                                                                       \
        __typeof__(*map_pairs(self)) *old_pairs = map_pairs(self);                                                     \
        map_pairs(self) = mm_alloc(sizeof(*map_pairs(self)) * (map_capacity_mo(self) + 1));                            \
        memset(map_pairs(self), 0, sizeof(*map_pairs(self)) * (map_capacity_mo(self) + 1));                            \
                                                                                                                       \
        for(array_count_t resize_i = 0; resize_i < map_capacity_mo(self) / 2; ++resize_i)                              \
            if(!pair_is_empty(old_pairs[resize_i]))                                                                    \
                map_insert_internal(self, pair_key_get, pair_is_empty, old_pairs[resize_i]);                           \
                                                                                                                       \
        mm_free(old_pairs);                                                                                            \
    })

#define map_insert(self, pair_key_get, pair_is_empty, elem)                                                            \
    __extension__({                                                                                                    \
        if(unlikely(map_count(self) * MAP_MAX_LOAD_XB > map_capacity_mo(self) * MAP_MAX_LOAD_XA))                      \
            map_expand(self, pair_key_get, pair_is_empty);                                                             \
        map_insert_internal(self, pair_key_get, pair_is_empty, elem);                                                  \
        ++map_count(self);                                                                                             \
    })

#define map_find(self, pair_key_get, pair_is_empty, key)                                                               \
    __extension__({                                                                                                    \
        array_count_t cmo = map_capacity_mo(self), l = (array_count_t)((key) & cmo), dib = 0;                          \
        __typeof__(*map_pairs(self)) *pairs = map_pairs(self), *ret = NULL;                                            \
        while(!pair_is_empty(pairs[l]) && ((l - pair_key_get(pairs[l])) & cmo) >= dib) {                               \
            if(pair_key_get(pairs[l]) == (key)) {                                                                      \
                ret = &pairs[l];                                                                                       \
                break;                                                                                                 \
            }                                                                                                          \
            ++dib;                                                                                                     \
            l = (l + 1) & cmo;                                                                                         \
        }                                                                                                              \
        ret;                                                                                                           \
    })

#define map_find_unsafe(self, pair_key_get, key)                                                                       \
    __extension__({                                                                                                    \
        array_count_t cmo = map_capacity_mo(self), l = (array_count_t)((key) & cmo);                                   \
        __typeof__(*map_pairs(self)) *pairs = map_pairs(self);                                                         \
        while(pair_key_get(pairs[l]) != (key))                                                                         \
            l = (l + 1) & cmo;                                                                                         \
        &pairs[l];                                                                                                     \
    })

#define map_erase(self, pair_key_get, pair_is_empty, key)                                                              \
    __extension__({                                                                                                    \
        array_count_t cmo = map_capacity_mo(self), l = (array_count_t)((key) & cmo);                                   \
        --map_count(self);                                                                                             \
        __typeof__(*map_pairs(self)) *pairs = map_pairs(self);                                                         \
        while(pair_key_get(pairs[l]) != (key))                                                                         \
            l = (l + 1) & cmo;                                                                                         \
                                                                                                                       \
        array_count_t k = l;                                                                                           \
        while(1) {                                                                                                     \
            array_count_t t = (k + 1) & cmo;                                                                           \
            if(pair_is_empty(pairs[t]) || !((t - pair_key_get(pairs[t])) & cmo))                                       \
                break;                                                                                                 \
            k = t;                                                                                                     \
        }                                                                                                              \
                                                                                                                       \
        if(likely(k >= l)) {                                                                                           \
            memmove(&pairs[l], &pairs[l + 1], sizeof(pairs[l]) * (k - l));                                             \
        } else {                                                                                                       \
            memmove(&pairs[l], &pairs[l + 1], sizeof(pairs[l]) * (cmo - l));                                           \
            pairs[cmo] = pairs[0];                                                                                     \
            memmove(&pairs[0], &pairs[1], sizeof(pairs[l]) * k);                                                       \
        }                                                                                                              \
        memset(&pairs[k], 0, sizeof(pairs[k]));                                                                        \
    })

#define map_for_each(self, pair_is_empty, var, fnc)                                                                    \
    __extension__({                                                                                                    \
        __typeof__(*map_pairs(self)) *pairs = map_pairs(self);                                                         \
        for(array_count_t k = 0; k <= map_capacity_mo(self); ++k) {                                                    \
            __typeof__(*map_pairs(self)) *var = &pairs[k];                                                             \
            if(!pair_is_empty(*var))                                                                                   \
                fnc;                                                                                                   \
        }                                                                                                              \
    })
