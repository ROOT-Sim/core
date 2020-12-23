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

#include <arch/io.h>
#include <arch/timer.h>
#include <core/arg_parse.h>
#include <core/core.h>

#include <inttypes.h>
#include <memory.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#define STATS_FOLDER_NAME_SUFFIX "_stats"
#define STATS_BUFFER_ENTRIES (512 * 1024 / sizeof(struct stats_info))

#ifdef ROOTSIM_MPI
#define is_stats_thread() (!rid && !nid)
#else
#define is_stats_thread() (!rid)
#endif

/// A set of statistical values of a single metric
/** The form of these values is designed for easier incremental updates */
struct stats_measure {
	/// The count of events of this type
	uint64_t count;
	/// The mean time to complete an event multiplied by the events count
	uint64_t sum_t;
	/// The variance of the time to complete multiplied by the events count
	uint64_t var_t;
};

/// A container for statistics in a logical time period
struct stats_info {
	/// The end of the logical time period for these statistics
	simtime_t gvt;
	/// The array of statistics taken in the period
	struct stats_measure s[STATS_NUM];
};

/// The statistics names, used to fill in the header of the final csv
const char * const s_names[] = {
	[STATS_ROLLBACK] = "rollbacks",
	[STATS_GVT] = "gvt",
	[STATS_MSG_SILENT] = "silent messages",
	[STATS_MSG_PROCESSED] = "processed messages"
};

static io_file_t *stats_tmp_fs;
static __thread struct stats_info stats_buf[STATS_BUFFER_ENTRIES];
static __thread struct stats_info *stats_cur;
static __thread timer_uint last_ts[STATS_NUM];

/**
 * @brief A version of the standard fopen() which accepts a printf style format
 * @param open_type a string which controls how the file is opened (check fopen())
 * @param fmt the file name expressed as a printf style format string
 * @param ... the list of additional arguments used in @a fmt (check printf())
 */
static FILE *file_open(const char *open_type, const char *fmt, ...)
{
	va_list args, args_cp;
	va_start(args, fmt);
	va_copy(args_cp, args);

	size_t l = vsnprintf(NULL, 0, fmt, args_cp) + 1;
	va_end(args_cp);

	char *f_name = mm_alloc(l);
	vsnprintf(f_name, l, fmt, args);
	va_end(args);

	FILE *ret = fopen(f_name, open_type);
	if (ret == NULL)
		log_log(LOG_ERROR, "Unable to open \"%s\" in %s mode", f_name,
			open_type);

	mm_free(f_name);
	return ret;
}

/**
 * @brief Flushes the stats buffer onto the temporary file
 */
static void flush_stats_buffer(void)
{
	int res = io_file_append(stats_tmp_fs[rid], stats_buf, sizeof(stats_buf));
	if (unlikely(res))
		log_log(LOG_ERROR, "Error during disk write!");

	memset(stats_buf, 0, sizeof(stats_buf));
}

/**
 * @brief Initializes the stats subsystem in the node
 */
void stats_global_init(void)
{
	stats_tmp_fs = mm_alloc(n_threads * sizeof(*stats_tmp_fs));
	for (rid_t i = 0; i < n_threads; ++i)
		stats_tmp_fs[i] = io_file_tmp_get();
}

/**
 * @brief Initializes the stats subsystem in the current thread
 */
void stats_init(void)
{
	stats_cur = stats_buf;
}

struct tmp_file_proc_args {
	FILE *o;
	nid_t n;
	rid_t r;
};

static void chunk_append_proc(size_t chunk_size, void *args_p)
{
	struct tmp_file_proc_args *args_pc = args_p;
	FILE *o = args_pc->o;
	nid_t n = args_pc->n;
	rid_t r = args_pc->r;
	size_t entries = chunk_size / sizeof(struct stats_info);
	for (unsigned j = 0; j < entries; ++j) {
		struct stats_info *s_info = &stats_buf[j];
		if (unlikely(!s_info->gvt))
			return;

		fprintf(o, "%d,%u,%lf", n, r, s_info->gvt);
		for (unsigned i = 0; i < STATS_NUM; ++i) {
			struct stats_measure *s_mes = &s_info->s[i];
			fprintf(o, ",%" PRIu64 ",%" PRIu64 ",%" PRIu64,
				s_mes->count, s_mes->sum_t, s_mes->var_t);
		}
		fprintf(o, "\n");
	}
}

/**
 * @brief Finalizes the stats subsystem in the current thread
 */
void stats_fini(void)
{
	flush_stats_buffer();
}

#ifdef ROOTSIM_MPI
static void receive_stats_files(FILE *o)
{
	for (nid_t j = 0; j < n_nodes; ++j) {
		rid_t t = 0;
		mpi_raw_data_blocking_rcv(&t, sizeof(t), j);
		for (rid_t i = 0; i < t; ++i) {
			struct tmp_file_proc_args args = {.o = o, .n = j, .r = i};
			int res;
			do {
				res = mpi_raw_data_blocking_rcv(
						stats_buf, sizeof(stats_buf), j);
				chunk_append_proc(res, &args);
			} while (res != sizeof(stats_buf));

			if (res > sizeof(stats_buf))
				log_log(LOG_ERROR, "Unexpectedly big message during stats propagation!");
		}
	}
}

static void file_send_proc(size_t chunk_size, void *args_p)
{
	(void) args_p;
	mpi_raw_data_blocking_send(stats_buf, chunk_size, 0);
}


static void send_stats_files(void)
{
	mpi_raw_data_blocking_send(&n_threads, sizeof(n_threads), 0);
	for (rid_t i = 0; i < n_threads; ++i) {
		int res = io_file_process(stats_tmp_fs[rid], stats_buf,
					  sizeof(stats_buf), file_send_proc, NULL);
		if (unlikely(res))
			log_log(LOG_ERROR, "Error during disk read!");
	}
}

#endif

/**
 * @brief Finalizes the stats subsystem in the node
 *
 * When finalizing this subsystem the master node formats and dumps his
 * statistics from his temporary files onto the final csv. Then, in a
 * distributed setting, he receives the slaves temporary files, formatting and
 * dumping their statistics as well.
 */
void stats_global_fini(void)
{
#ifdef ROOTSIM_MPI
	mpi_node_barrier();
	if (nid) {
		send_stats_files();
		return;
	}
#endif
	FILE *o = file_open("w", "%s_stats.csv", arg_parse_program_name());
	if (o == NULL) {
		log_log(LOG_WARN, "Unavailable stats file: stats will be printed on the terminal");
		o = stdout;
	}

	fprintf(o, "node id,resource id,gvt");
	for (unsigned i = 0; i < STATS_NUM; ++i) {
		fprintf(o, ",%s count,%s time sum,%s cumulative time variance",
			s_names[i], s_names[i], s_names[i]);
	}
	fprintf(o, "\n");

	for (rid_t i = 0; i < n_threads; ++i) {
		struct tmp_file_proc_args args = {.o = o, .n = nid, .r = i};

		int res = io_file_process(stats_tmp_fs[i], stats_buf,
					  sizeof(stats_buf), chunk_append_proc,
					  &args);
		if (unlikely(res))
			log_log(LOG_ERROR, "Error during disk read!");
	}

#ifdef ROOTSIM_MPI
	receive_stats_files(o);
#endif
	if (o != stdout)
		fclose(o);
}

void stats_time_start(enum stats_time_t this_stat)
{
	last_ts[this_stat] = timer_new();
}

void stats_time_take(enum stats_time_t this_stat)
{
	struct stats_measure *s_mes = &stats_cur->s[this_stat];
	const uint64_t t = timer_value(last_ts[this_stat]);

	if (likely(s_mes->count)) {
		const int64_t num = (t * s_mes->count - s_mes->sum_t);
		s_mes->var_t += (num * num) /
			(s_mes->count * (s_mes->count + 1));
	}

	s_mes->sum_t += t;
	s_mes->count++;
}

void stats_on_gvt(simtime_t gvt)
{
	stats_cur->gvt = gvt;
	++stats_cur;

	if (unlikely(stats_buf + STATS_BUFFER_ENTRIES == stats_cur)) {
		flush_stats_buffer();
		stats_cur = stats_buf;
	}

	if (!is_stats_thread())
		return;

	printf("\rVirtual time: %lf", gvt);
	fflush(stdout);
}

void stats_dump(void)
{
	puts("");
	fflush(stdout);
}
