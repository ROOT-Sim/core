/**
 * @file log/stats.c
 *
 * @brief Statistics module
 *
 * All facilities to collect, gather, and dump statistics are implemented in this module.
 *
 * SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <log/stats.h>

#include <arch/io.h>
#include <arch/mem.h>
#include <arch/timer.h>
#include <distributed/mpi.h>
#include <log/file.h>
#include <mm/mm.h>


#include <assert.h>
#include <stdint.h>
#include <stdio.h>

/// The number of entries to keep in the stdio buffers before flushing a temporary statistics file to disk
#define STATS_BUFFER_ENTRIES (1024)

/// A container for statistics in a logical time period
struct stats_thread {
	/// The array of statistics taken in the period
	uint64_t s[STATS_COUNT];
};

/// A container for node-wide statistics in a logical time period
struct stats_node {
	/// The gvt value
	simtime_t gvt;
	/// The size in bytes of the resident set
	uint64_t rss;
};

/// A container for node-wide statistics relevant for the whole simulation run
struct stats_global {
	/// The number of threads in this node
	uint64_t threads_count;
	/// The maximum size in bytes of the resident set
	uint64_t max_rss;
	/// The timestamps of the relevant simulation life-cycle events
	uint64_t timestamps[STATS_GLOBAL_COUNT];
};

static_assert(sizeof(struct stats_thread) == 8 * STATS_COUNT && sizeof(struct stats_node) == 16 &&
		  sizeof(struct stats_global) == 16 + 8 * (STATS_GLOBAL_COUNT),
    "structs aren't properly packed, parsing may be difficult");

/// The statistics names, used to fill in the preamble of the final statistics binary file
const char *const stats_names[] = {
    [STATS_ROLLBACK] = "rollbacks",
    [STATS_MSG_ROLLBACK] = "rolled back messages",
    [STATS_MSG_REMOTE_RECEIVED] = "remote messages received",
    [STATS_MSG_SILENT] = "silent messages",
    [STATS_CKPT] = "checkpoints",
    [STATS_CKPT_TIME] = "checkpoints time",
    [STATS_CKPT_STATE_SIZE] = "checkpoints state size",
    [STATS_MSG_SILENT_TIME] = "silent messages time",
    [STATS_MSG_PROCESSED] = "processed messages",
    [STATS_REAL_TIME_GVT] = "gvt real time"
};

/// The first timestamp ever collected for this simulation run
static timer_uint sim_start_ts;
static timer_uint sim_start_ts_hr;
/// The global stats
static struct stats_global stats_glob_cur;
/// A pointer to the temporary file used to save #stats_node structs produced during simulation
static FILE *stats_node_tmp;
/// An array of pointers to the temporary files used to save #stats_thread structs produced by threads during simulation
static FILE **stats_tmps;
/// The current values of thread statistics for this logical time period (from the previous GVT to the next one)
static __thread struct stats_thread stats_cur;

/**
 * @brief Take a lifetime event time value
 * @param this_stat The type of event just occurred
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
	sim_start_ts = timer_new();
	sim_start_ts_hr = timer_hr_new();

	if(global_config.stats_file == NULL)
		return;

	stats_node_tmp = io_file_tmp_get();
	if(unlikely(stats_node_tmp == NULL)) {
		logger(LOG_ERROR, "Unable to open a temporary file, statistics won't be collected");
		global_config.stats_file = NULL;
		return;
	}

	setvbuf(stats_node_tmp, NULL, _IOFBF, STATS_BUFFER_ENTRIES * sizeof(struct stats_node));

	if(mem_stat_setup() < 0)
		logger(LOG_ERROR, "Unable to extract memory statistics!");
	stats_tmps = mm_alloc(global_config.n_threads * sizeof(*stats_tmps));
	stats_glob_cur.threads_count = global_config.n_threads;
}

/**
 * @brief Initializes the stats subsystem in the current thread
 */
void stats_init(void)
{
	if(global_config.stats_file == NULL)
		return;

	stats_tmps[rid] = io_file_tmp_get();
	if(unlikely(stats_tmps[rid] == NULL)) {
		logger(LOG_ERROR, "Unable to open a temporary file, statistics won't be collected");
		global_config.stats_file = NULL;
		return;
	}

	setvbuf(stats_tmps[rid], NULL, _IOFBF, STATS_BUFFER_ENTRIES * sizeof(stats_cur));
}

/**
 * @brief Collect and dump the statistics from other nodes in the final binary file
 * @param[out] out_f a pointer to the file to write. This can be NULL: the data from other nodes won't be saved
 *
 * You may want to call this function with @p out_f set to NULL to flush the pending MPI communications.
 * For details about the binary file format see #stats_file_final_write().
 */
static void stats_files_receive(FILE *out_f)
{
	for(nid_t j = 1; j < n_nodes; ++j) {
		int buf_size;
		struct stats_global *sg_p = mpi_blocking_data_rcv(&buf_size, j);
		if(likely(out_f != NULL))
			file_write_chunk(out_f, sg_p, buf_size);
		uint64_t iters = sg_p->threads_count + 1; // +1 for node stats
		mm_free(sg_p);

		for(uint64_t i = 0; i < iters; ++i) {
			void *buf = mpi_blocking_data_rcv(&buf_size, j);
			int64_t f_size = buf_size;
			if(likely(out_f != NULL)) {
				file_write_chunk(out_f, &f_size, sizeof(f_size));
				file_write_chunk(out_f, buf, buf_size);
			}
			mm_free(buf);
		}
	}
}

/**
 * @brief Send the final statistics data of this node to the master node
 */
static void stats_files_send(void)
{
	stats_glob_cur.max_rss = mem_stat_rss_max_get();
	stats_glob_cur.timestamps[STATS_GLOBAL_END] = timer_value(sim_start_ts);
	stats_glob_cur.timestamps[STATS_GLOBAL_HR_TOTAL] = timer_hr_value(sim_start_ts_hr);
	mpi_blocking_data_send(&stats_glob_cur, sizeof(stats_glob_cur), 0);

	int64_t f_size;
	void *f_buf = file_memory_load(stats_node_tmp, &f_size);
	f_size = min(INT_MAX, f_size);
	mpi_blocking_data_send(f_buf, f_size, 0);
	mm_free(f_buf);

	for(rid_t i = 0; i < global_config.n_threads; ++i) {
		f_buf = file_memory_load(stats_tmps[i], &f_size);
		f_size = min(INT_MAX, f_size);
		mpi_blocking_data_send(f_buf, f_size, 0);
		mm_free(f_buf);
	}
}

/**
 * @brief Write the final statistic file, except for the data from other nodes
 * @param[out] out_f a pointer to the file to write
 *
 * The following tables document the content of the final generated statistics file.
 * - Rows are in order of appearance in the binary file
 * - The binary file starts with a single Preamble
 * - The Count column specifies how many repeated elements of that row are present
 * - The Size column reports the size of a single row element in bytes
 * - The Type column specifies the type of the field: uppercase types are further described in their own table
 * - The Ref column assigns an identifier for values that are used later on to define counts of other rows
 * - The Description column documents the purpose of the field
 *
 * Preamble:
 * | Count      | Size | Type             | Ref   | Description                                                        |
 * |:---------- |:---- |:---------------- |:----- | :----------------------------------------------------------------- |
 * | 1          | 2    | uint             | --    | Magic number used to detect the endianness of the machine          |
 * | 1          | 8    | int              | s_cnt | Count of available thread metrics                                  |
 * | s_cnt      | *    | Pascal string    | --    | Names of the thread metrics                                        |
 * | 1          | 8    | int              | n_cnt | Count of MPI ranks (1 in the case of a single node run)            |
 * | n_cnt      | *    | Node stats       | --    | The statistics produced by the nodes                               |
 *
 * Pascal string:
 * | Count      | Size | Type             | Ref   | Description                                                        |
 * |:---------- |:---- |:---------------- |:---   | :----------------------------------------------------------------- |
 * | 1          | 1    | uint             | p_len | Size of the Pascal string (maximum 255, clearly)                   |
 * | p_len      | 1    | char             | --    | Content of the string                                              |
 *
 * Node stats:
 * | Count      | Size | Type             | Ref   | Description                                                        |
 * |:---------- |:---- |:---------------- |:----- | :----------------------------------------------------------------- |
 * | 1          | 8    | uint             | t_cnt | Count of threads for the this node                                 |
 * | 1          | 8    | uint             | --    | Maximum resident set size of this node (in bytes)                  |
 * | 6          | 8    | uint             | --    | Some timestamps in us (see enum #stats_global_type)                |
 * | 1          | 8    | uint             | --    | High resolution time end to end value (see enum #stats_global_type)|
 * | 1          | 8    | int              | n_siz | Size of the node GVT stats array                                   |
 * | n_siz / 16 | 16   | Node GVT entry   | --    | The node-wide statistics produced at each GVT by this node         |
 * | t_cnt      | *    | Thread GVT stats | --    | The statistics produced by each thread on this node                |
 *
 * Node GVT entry:
 * | Count      | Size | Type             | Ref   | Description                                                        |
 * |:---------- |:---- |:---------------- |:----- | :----------------------------------------------------------------- |
 * | 1          | 8    | double           | --    | Global virtual time value                                          |
 * | 1          | 8    | uint             | --    | Current resident set size of this node (in bytes)                  |
 *
 * Thread stats:
 * | Count               | Size      | Type             | Ref   | Description                                          |
 * |:------------------- |:--------- |:---------------- |:----- | :--------------------------------------------------- |
 * | 1                   | 8         | uint             | t_siz | Size in bytes of this thread's GVT stats array       |
 * | t_siz / (s_cnt * 8) | s_cnt * 8 | Thread GVT entry | --    |  The statistics produced at each GVT by this thread  |
 *
 * Thread GVT entry:
 * | Count      | Size | Typ   | Ref   | Description                                                                   |
 * |:---------- |:---- |:----- |:----- | :---------------------------------------------------------------------------- |
 * | s_cnt      | 8    | uint  | --    | The values of the stats described in the Preamble for this GVT and thread     |
 *
 * In a correctly completed simulation n_siz / 16 == t_siz / (s_cnt * 8) for each node and thread (this is the number of
 * committed GVTs). This function only writes the Node stats for the current node (the master node in a MPI run). The
 * function #stats_files_receive() deals with the other nodes.
 * TODO add to the file other kind of statistics, for example ROOT-Sim config, machine hardware etc
 */
static void stats_file_final_write(FILE *out_f)
{
	uint16_t endian_check = 61455U;
	file_write_chunk(out_f, &endian_check, sizeof(endian_check));

	int64_t n = STATS_COUNT;
	file_write_chunk(out_f, &n, sizeof(n));
	for(int i = 0; i < STATS_COUNT; ++i) {
		unsigned char l = strnlen(stats_names[i], UCHAR_MAX);
		file_write_chunk(out_f, &l, 1);
		file_write_chunk(out_f, stats_names[i], l);
	}

	n = n_nodes;
	file_write_chunk(out_f, &n, sizeof(n));

	stats_glob_cur.max_rss = mem_stat_rss_max_get();
	stats_glob_cur.timestamps[STATS_GLOBAL_END] = timer_value(sim_start_ts);
	stats_glob_cur.timestamps[STATS_GLOBAL_HR_TOTAL] = timer_hr_value(sim_start_ts_hr);
	file_write_chunk(out_f, &stats_glob_cur, sizeof(stats_glob_cur));

	int64_t buf_size;
	void *buf = file_memory_load(stats_node_tmp, &buf_size);
	file_write_chunk(out_f, &buf_size, sizeof(buf_size));
	file_write_chunk(out_f, buf, buf_size);
	mm_free(buf);

	for(rid_t i = 0; i < global_config.n_threads; ++i) {
		buf = file_memory_load(stats_tmps[i], &buf_size);
		file_write_chunk(out_f, &buf_size, sizeof(buf_size));
		file_write_chunk(out_f, buf, buf_size);
		mm_free(buf);
	}
}

/**
 * @brief Finalizes the stats subsystem in the node
 *
 * When finalizing this subsystem, the master node dumps his statistics from his temporary files onto the  final binary
 * file. Then, in a distributed setting, he receives the slaves temporary files, dumping their statistics as well.
 */
void stats_global_fini(void)
{
	if(global_config.stats_file == NULL)
		return;

	if(nid) {
		stats_files_send();
	} else {
		FILE *o = file_open("w", "%s.bin", global_config.stats_file);
		if(unlikely(o == NULL)) {
			logger(LOG_WARN, "Unable to open statistics file for writing, statistics won't be saved.");
			stats_files_receive(NULL);
		} else {
			stats_file_final_write(o);
			stats_files_receive(o);
			fclose(o);
		}
	}

	for(rid_t i = 0; i < global_config.n_threads; ++i)
		fclose(stats_tmps[i]);

	mm_free(stats_tmps);
	fclose(stats_node_tmp);
}

/**
 * @brief Sum a sample to a statistics value
 *
 * @param this_stat the statistics type to add the sample to
 * @param c the sample to sum
 */
void stats_take(enum stats_thread_type this_stat, unsigned c)
{
	stats_cur.s[this_stat] += c;
}

/**
 * @brief Perform GVT related activities for the statistics subsystem
 *
 * @param gvt the time value of the current GVT
 *
 * Dumps accumulated statistics to the file and resets the statistics buffer to
 * ready up for the following processing phase
 */
void stats_on_gvt(simtime_t gvt)
{
	if(global_config.log_level != LOG_SILENT && !rid && !nid) {
		if(unlikely(gvt == SIMTIME_MAX))
			printf("\rVirtual time: infinity");
		else
			printf("\rVirtual time: %lf", gvt);

		fflush(stdout);
	}

	if(global_config.stats_file == NULL)
		return;

	stats_cur.s[STATS_REAL_TIME_GVT] = timer_value(sim_start_ts);

	file_write_chunk(stats_tmps[rid], &stats_cur, sizeof(stats_cur));
	memset(&stats_cur, 0, sizeof(stats_cur));

	if(rid != 0)
		return;

	struct stats_node stats_node_cur = {.gvt = gvt, .rss = mem_stat_rss_current_get()};
	file_write_chunk(stats_node_tmp, &stats_node_cur, sizeof(stats_node_cur));
	memset(&stats_node_cur, 0, sizeof(stats_node_cur));
}

/**
 * @brief Dump some final minimal statistics on screen
 */
void stats_dump(void)
{
	if(nid == 0) {
		if(global_config.log_level != LOG_SILENT) {
			puts("");
			fflush(stdout);
		}
		double t = (double)timer_value(sim_start_ts) / 1000000.0;
		logger(LOG_INFO, "Simulation completed in %.3lf seconds", t);
	}
}

/**
 * @brief Retrieve the value of a metric of this thread
 *
 * This values are computed since the end of the last GVT.
 */
uint64_t stats_retrieve(enum stats_thread_type this_stat)
{
	return stats_cur.s[this_stat];
}
