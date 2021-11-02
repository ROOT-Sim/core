/**
 * @file logger/stats.c
 *
 * @brief Statistics module
 *
 * All facilities to collect, gather, and dump statistics are implemented
 * in this module.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <log/stats.h>

#include <arch/io.h>
#include <arch/mem.h>
#include <arch/timer.h>
#include <core/core.h>
#include <distributed/mpi.h>

#include <assert.h>
#include <inttypes.h>
#include <memory.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#define STATS_BUFFER_ENTRIES (1024)
#define STATS_MAX_STRLEN 32U
#define STATS_REAL_TIME STATS_COUNT

/// A container for statistics in a logical time period
struct stats_thread {
	/// The array of statistics taken in the period
	uint64_t s[STATS_COUNT];
};

struct stats_node {
	/// The gvt value
	simtime_t gvt;
	/// The size in bytes of the resident set
	uint64_t rss;
};

struct stats_global {
	/// The number of threads in this node
	uint64_t threads_count;
	/// The maximum size in bytes of the resident set
	uint64_t max_rss;
	/// The timestamps of the relevant simulation life-cycle events
	uint64_t timestamps[STATS_GLOBAL_COUNT];
};

static_assert(sizeof(struct stats_thread) == 8 * STATS_COUNT &&
	      sizeof(struct stats_node) == 16 &&
	      sizeof(struct stats_global) == 16 + 8 * (STATS_GLOBAL_COUNT),
	      "structs aren't naturally packed, parsing may be difficult");

/// The statistics names, used to fill in the header of the final csv
const char * const s_names[] = {
	[STATS_ROLLBACK] = "rollbacks",
	[STATS_MSG_ROLLBACK] = "rollbacked messages",
	[STATS_MSG_REMOTE_RECEIVED] = "remote messages received",
	[STATS_MSG_SILENT] = "silent messages",
	[STATS_CKPT] = "checkpoints",
	[STATS_CKPT_TIME] = "checkpoints time",
	[STATS_MSG_SILENT_TIME] = "silent messages time",
	[STATS_MSG_PROCESSED] = "processed messages",
	[STATS_REAL_TIME_GVT] = "gvt real time"
};

static timer_uint sim_start_ts;
static struct stats_global stats_glob_cur;

static FILE *stats_node_tmp;
static FILE **stats_tmps;
static __thread struct stats_thread stats_cur;

static void file_write_chunk(FILE *f, const void *data, size_t data_size)
{
	if (unlikely(fwrite(data, data_size, 1, f) != 1))
		logger(LOG_ERROR, "Error during disk write!");
}

static void *file_memory_load(FILE *f, int64_t *f_size_p)
{
	fseek(f, 0, SEEK_END);
	long f_size = ftell(f); // Fails horribly for files bigger than 2 GB
	fseek(f, 0, SEEK_SET);
	void *ret = mm_alloc(f_size);
	if (fread(ret, f_size, 1, f) != 1) {
		mm_free(ret);
		*f_size_p = 0;
		return NULL;
	}
	*f_size_p = f_size;
	return ret;
}

/**
 * @brief A version of fopen() which accepts a printf style format string
 * @param open_type a string which controls how the file is opened (see fopen())
 * @param fmt the file name expressed as a printf style format string
 * @param ... the list of additional arguments used in @a fmt (see printf())
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
		logger(LOG_ERROR, "Unable to open \"%s\" in %s mode", f_name, open_type);

	mm_free(f_name);
	return ret;
}

/**
 * @brief Initializes the internal timer used to take accurate measurements
 */
void stats_global_time_start(void)
{
	sim_start_ts = timer_new();
}

/**
 * @brief Initializes the internal timer used to take accurate measurements
 */
void stats_global_time_take(enum stats_global_type this_stat)
{
	stats_glob_cur.timestamps[this_stat] = timer_value(sim_start_ts);
}

/**
 * @brief Initializes the stats subsystem in the node
 */
void stats_global_init(void)
{
	stats_glob_cur.timestamps[STATS_GLOBAL_START] = timer_value(sim_start_ts);
	stats_glob_cur.threads_count = n_threads;
	if (mem_stat_setup() < 0)
		logger(LOG_ERROR, "Unable to extract memory statistics!");

	stats_node_tmp = io_file_tmp_get();
	setvbuf(stats_node_tmp, NULL, _IOFBF,
		STATS_BUFFER_ENTRIES * sizeof(struct stats_node));

	stats_tmps = mm_alloc(n_threads * sizeof(*stats_tmps));
}

/**
 * @brief Initializes the stats subsystem in the current thread
 */
void stats_init(void)
{
	stats_tmps[rid] = io_file_tmp_get();
	setvbuf(stats_tmps[rid], NULL, _IOFBF,
		STATS_BUFFER_ENTRIES * sizeof(stats_cur));
}

static void stats_files_receive(FILE *o)
{
	for (nid_t j = 1; j < n_nodes; ++j) {
		int buf_size;
		struct stats_global *sg_p = mpi_blocking_data_rcv(&buf_size, j);
		file_write_chunk(o, sg_p, buf_size);
		uint64_t iters = sg_p->threads_count + 1; // +1 for node stats
		mm_free(sg_p);

		for (uint64_t i = 0; i < iters; ++i) {
			void *buf = mpi_blocking_data_rcv(&buf_size, j);
			int64_t f_size = buf_size;
			file_write_chunk(o, &f_size, sizeof(f_size));
			file_write_chunk(o, buf, buf_size);
			mm_free(buf);
		}
	}
}

static void stats_files_send(void)
{
	stats_glob_cur.max_rss = mem_stat_rss_max_get();
	stats_glob_cur.timestamps[STATS_GLOBAL_END] = timer_value(sim_start_ts);
	mpi_blocking_data_send(&stats_glob_cur, sizeof(stats_glob_cur), 0);

	int64_t f_size;
	void *f_buf = file_memory_load(stats_node_tmp, &f_size);
	f_size = min(INT_MAX, f_size);
	mpi_blocking_data_send(f_buf, f_size, 0);
	mm_free(f_buf);

	for (rid_t i = 0; i < n_threads; ++i) {
		f_buf = file_memory_load(stats_tmps[i], &f_size);
		f_size = min(INT_MAX, f_size);
		mpi_blocking_data_send(f_buf, f_size, 0);
		mm_free(f_buf);
	}
}

// TODO add other statistics, for example ROOT-Sim config, machine hardware etc
static void stats_file_final_write(FILE *o)
{
	uint16_t endian_check = 61455U; // 0xFOOF
	file_write_chunk(o, &endian_check, sizeof(endian_check));

	int64_t n = STATS_COUNT;
	file_write_chunk(o, &n, sizeof(n));
	for (int i = 0; i < STATS_COUNT; ++i) {
		size_t l = min(strlen(s_names[i]), STATS_MAX_STRLEN);
		file_write_chunk(o, s_names[i], l);
		unsigned char nul = 0;
		for (; l < STATS_MAX_STRLEN; ++l)
			file_write_chunk(o, &nul, 1);
	}

	n = n_nodes;
	file_write_chunk(o, &n, sizeof(n));

	stats_glob_cur.max_rss = mem_stat_rss_max_get();
	stats_glob_cur.timestamps[STATS_GLOBAL_END] = timer_value(sim_start_ts);
	file_write_chunk(o, &stats_glob_cur, sizeof(stats_glob_cur));

	int64_t buf_size;
	void *buf = file_memory_load(stats_node_tmp, &buf_size);
	file_write_chunk(o, &buf_size, sizeof(buf_size));
	file_write_chunk(o, buf, buf_size);
	mm_free(buf);

	for (rid_t i = 0; i < n_threads; ++i) {
		buf = file_memory_load(stats_tmps[i], &buf_size);
		file_write_chunk(o, &buf_size, sizeof(buf_size));
		file_write_chunk(o, buf, buf_size);
		mm_free(buf);
	}
}

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
	if (nid) {
		stats_files_send();
		return;
	}

	FILE *o = file_open("w", "%s_stats.bin", "TODO"); // TODO: get back the name of the executable
	if (o == NULL) {
		logger(LOG_WARN, "Unavailable stats file: stats will be dumped on stdout");
		o = stdout;
	}

	stats_file_final_write(o);
	stats_files_receive(o);

	fflush(o);
	if (o != stdout)
		fclose(o);

	for (rid_t i = 0; i < n_threads; ++i)
		fclose(stats_tmps[i]);

	mm_free(stats_tmps);
	fclose(stats_node_tmp);
}

void stats_take(enum stats_thread_type this_stat, unsigned c)
{
	stats_cur.s[this_stat] += c;
}

void stats_on_gvt(simtime_t gvt)
{
	stats_cur.s[STATS_REAL_TIME_GVT] = timer_value(sim_start_ts);

	file_write_chunk(stats_tmps[rid], &stats_cur, sizeof(stats_cur));
	memset(&stats_cur, 0, sizeof(stats_cur));

	if (rid != 0)
		return;

	struct stats_node stats_node_cur =
			{.gvt = gvt, .rss = mem_stat_rss_current_get()};
	file_write_chunk(stats_node_tmp, &stats_node_cur, sizeof(stats_node_cur));
	memset(&stats_node_cur, 0, sizeof(stats_node_cur));

	if (nid != 0)
		return;

	printf("\rVirtual time: %lf", gvt);
	fflush(stdout);
}

void stats_dump(void)
{
	if (nid == 0) {
		double t = timer_value(sim_start_ts) / 1000000.0;
		printf("\nSimulation completed in %.3lf seconds\n", t);
		fflush(stdout);
	}
}

uint64_t stats_retrieve(enum stats_thread_type this_stat)
{
	return stats_cur.s[this_stat];
}
