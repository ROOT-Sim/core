/**
 * @file log/stats.c
 *
 * @brief Statistics module
 *
 * All facitilies to collect, gather, and dump statistics are implemented
 * in this module.
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://hpdcs.github.io
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
 * @todo Still missing fine-grained stats information
 */
#include <log/stats.h>

#include <inttypes.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#include <core/timer.h>

struct stats_info {
	uint64_t count;
	uint64_t sum_t;
	uint64_t var_t;
};

static __thread struct stats_info thread_stats[STATS_NUM];

static __thread uint64_t last_ts[STATS_NUM];

static struct stats_info *to_aggregate;

void stats_global_init(void)
{
	to_aggregate = mm_alloc(sizeof(thread_stats) * n_threads);
}

void stats_time_start(enum stats_time_t this_stat)
{
	last_ts[this_stat] = timer_new();
}

void stats_time_take(enum stats_time_t this_stat)
{
	struct stats_info *s_info = &thread_stats[this_stat];
	const uint64_t t = timer_value(last_ts[this_stat]);

	if(s_info->count){
		const int64_t num = (t * s_info->count - s_info->sum_t);
		s_info->var_t += (num * num) / (s_info->count *
			(s_info->count + 1));
	}

	s_info->sum_t += t;
	s_info->count++;
}

void stats_dump(void)
{
	if (!rid) {
		printf("\n");
		log_log(LOG_INFO, "Simulation completed!");
	}
}

void stats_on_gvt(simtime_t current_gvt)
{
	if (!rid) {
		printf("\rVirtual time: %lf", current_gvt);
		fflush(stdout);
	}
}

/*
static void stats_threads_reduce(void)
{
	struct stats_info node_stats[STATS_NUM];
	unsigned thr_cnt = n_threads;
	for(unsigned i = 0; i < thr_cnt; ++i){
		for(unsigned j = 0; j < STATS_NUM; ++j){
			node_stats[j].count +=
					to_aggregate[STATS_NUM * i + j].count;
			node_stats[j].sum_t +=
					to_aggregate[STATS_NUM * i + j].sum_t;
			node_stats[j].var_t +=
					to_aggregate[STATS_NUM * i + j].var_t;
		}
	}
	for(unsigned i = 0; i < thr_cnt; ++i){
		for(unsigned j = 0; j < STATS_NUM; ++j){ // correct but obviously truncates everything FIXME
			const int64_t sum =
				to_aggregate[STATS_NUM * i + j].sum_t * node_stats[j].count -
				to_aggregate[STATS_NUM * i + j].count * node_stats[j].sum_t;

			node_stats[j].var_t += (sum * sum) /
				(to_aggregate[STATS_NUM * i + j].count * node_stats[j].count * node_stats[j].count);
		}
	}
}

void stats_reduce(void)
{
	struct stats_info *s_info = thread_stats;
	static atomic_uint t_count = 0;

	memcpy(&to_aggregate[STATS_NUM * rid], thread_stats, sizeof(thread_stats));

	if (atomic_fetch_add_explicit(
		&t_count, 1U, memory_order_relaxed) == n_threads){
		stats_threads_reduce();
		atomic_store_explicit(&t_count, 0U, memory_order_relaxed);
	}

	memset(thread_stats, 0, sizeof(thread_stats));
}

*/
