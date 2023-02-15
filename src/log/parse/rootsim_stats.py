#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

import copy
import struct
import sys


##
# @brief The statistics class used to load and pre-process ROOT-Sim statistics files
#
# If you configured ROOT-Sim to produce a statistics file, a binary file will be generated.
# You can use the facilities offered by this class to efficiently load those statistics.
class RSStats:
    def _pattern_unpack(self, ptrn):
        ptrn_size = struct.calcsize(ptrn)
        data_parse = self._data[self._data_idx:ptrn_size + self._data_idx]
        self._data_idx += ptrn_size
        ret = struct.unpack((">" if self.big_endian else "<") + ptrn, data_parse)
        return ret

    def _metric_names_load(self):
        metric_names = []
        n_stats = self._pattern_unpack("q")[0]
        for _ in range(n_stats):
            raw_name_len = self._pattern_unpack("B")[0]
            raw_name = self._pattern_unpack(f"{raw_name_len}s")[0]
            metric_name = raw_name.decode("utf-8").rstrip('\0')
            metric_names.append(metric_name)
            self._metrics[metric_name] = []
        return metric_names

    def _threads_unpack(self):
        metrics_len = len(self.metrics)
        n_stats = self._pattern_unpack("q")[0] // (metrics_len * 8)
        t_stats_fmt = str(metrics_len) + "Q"
        ret = []
        for _ in range(n_stats):
            ret.append(self._pattern_unpack(t_stats_fmt))
        return ret

    def _nodes_stats_load(self):
        nodes_count = self._pattern_unpack("q")[0]
        for _ in range(nodes_count):
            glob_stats = self._pattern_unpack("9Q")
            n_threads = glob_stats[0]
            self.threads_count.append(n_threads)
            n_stats = self._pattern_unpack("q")[0] // 16
            node_stats = []
            for _ in range(n_stats):
                node_stats.append(self._pattern_unpack("dQ"))

            threads_stats = []
            for _ in range(n_threads):
                threads_stats.append(self._threads_unpack())

            self.all_stats.append((glob_stats, node_stats, threads_stats))

    def _truncate_to_last_gvt(self):
        min_gvts = len(self.all_stats[0][1])
        for _, node_stats, threads_stats in self.all_stats:
            min_gvts = min(len(node_stats), min_gvts)
            for t_stats in threads_stats:
                min_gvts = min(len(t_stats), min_gvts)

        ret = []
        for glob_stats, node_stats, threads_stats in self.all_stats:
            t_node_stats = node_stats[:min_gvts]
            t_threads_stats = []
            for t_stats in threads_stats:
                t_threads_stats.append(t_stats[:min_gvts])
            ret.append((glob_stats, t_node_stats, t_threads_stats))

        self.all_stats = ret

    ##
    # @brief Construct a new RSStats object
    # @param self the RSStats instance being constructed
    # @param rs_stats_file the path of the binary statistics file generated by ROOT-Sim
    def __init__(self, rs_stats_file):
        with open(rs_stats_file, 'rb') as bin_file:
            self._data = bin_file.read()

        self._data_idx = 0
        self.big_endian = True
        magic_number = self._pattern_unpack("H")[0]
        if magic_number not in (4080, 61455):
            raise RuntimeWarning("Parsing failed, wrong initial magic number")

        self.big_endian = magic_number == 61455
        self._metrics = {}
        metric_names = self._metric_names_load()

        self.threads_count = []
        self.all_stats = []
        self._nodes_stats_load()

        if len(self._data) != self._data_idx:
            raise RuntimeWarning("Parsing failed, there's garbage at the end")

        self._gvts = []
        self._truncate_to_last_gvt()

        self._global_measures = {
            "lps": [],
            "maximum_resident_set": [],
            "node_init_time": [],
            "worker_threads_init_time": [],
            "processing_time": [],
            "worker_threads_fini_time": [],
            "node_fini_time": [],
            "node_total_time": [],
            "node_total_hr_time": [],
            "resident_set": []
        }
        for triple in self.all_stats:
            glob_stats, node_stats, threads_stats = triple

            self._global_measures["lps"].append(glob_stats[1])
            self._global_measures["maximum_resident_set"].append(glob_stats[2])
            self._global_measures["node_init_time"].append(glob_stats[3])
            self._global_measures["worker_threads_init_time"].append(glob_stats[4] - glob_stats[3])
            self._global_measures["processing_time"].append(glob_stats[5] - glob_stats[4])
            self._global_measures["worker_threads_fini_time"].append(glob_stats[6] - glob_stats[5])
            self._global_measures["node_fini_time"].append(glob_stats[7] - glob_stats[6])
            self._global_measures["node_total_time"].append(glob_stats[7] - glob_stats[3])
            self._global_measures["node_total_hr_time"].append(glob_stats[8])

            mem = []
            for i, (gvt, crs_mem) in enumerate(node_stats):
                if len(self._gvts) <= i:
                    self._gvts.append(gvt)
                elif self._gvts[i] != gvt:
                    raise RuntimeWarning("Parsing failed, inconsistent GVTs across nodes and/or threads")

                mem.append(crs_mem)

            self._global_measures["resident_set"].append(mem)

            for j, metric_name in enumerate(metric_names):
                metric_n_stat = []
                for t_stat in threads_stats:
                    metric_t_stat = []
                    for tup in t_stat:
                        metric_t_stat.append(tup[j])
                    metric_n_stat.append(metric_t_stat)

                self._metrics[metric_name].append(metric_n_stat)

    ##
    # @brief Get the GVT values
    # @return a list containing the computed GVTs (Global Virtual Times) in ascending order
    @property
    def gvts(self):
        return list(self._gvts)

    ##
    # @brief Get the real time values
    # @return a list containing the computed real times in ascending order
    def rts(self, reduction=min):
        real_times = self._metrics["gvt real time"]
        ret = []
        for i in range(len(self._gvts)):
            values = []
            for node_stats in real_times:
                for thread_stats in node_stats:
                    values.append(thread_stats[i])
            ret.append(reduction(values))
        return ret

    ##
    # @brief Get the thread-specific metric names
    # @return a list containing the metric names that you can use in #thread_metric_get()
    @property
    def metrics(self):
        return list(self._metrics)

    ##
    # @brief Get the node-specific statistics
    # @return a dictionary containing node-specific statistics
    # TODO more thorough description of what we have here
    @property
    def nodes_stats(self):
        return self._global_measures

    @property
    def nodes_count(self):
        return len(self.threads_count)

    ##
    # @brief Get the thread-specific metric values
    # @return a list of values
    # FIXME: much more complicated, explain it!
    # TODO: optionally provide stats normalized by real-time
    def thread_metric_get(self, metric, aggregate_gvts=False, aggregate_threads=False, aggregate_nodes=False):
        if metric not in self._metrics:
            raise RuntimeError(f"Asked stats for the non-existing thread_metric {metric}")

        if aggregate_nodes:
            aggregate_threads = True

        this_stats = copy.deepcopy(self._metrics[metric])

        if aggregate_gvts:
            for nstats in this_stats:
                for i, _ in enumerate(nstats):
                    nstats[i] = sum(nstats[i])

            if aggregate_threads:
                for i, _ in enumerate(this_stats):
                    this_stats[i] = sum(this_stats[i])

                if aggregate_nodes:
                    this_stats = sum(this_stats)
        else:
            if aggregate_threads:
                for i, _ in enumerate(this_stats):
                    gvt_stats = [0] * len(self._gvts)
                    for rstats in this_stats[i]:
                        for j, val in enumerate(rstats):
                            gvt_stats[j] += val
                    this_stats[i] = gvt_stats

                if aggregate_nodes:
                    gvt_stats = [0] * len(self._gvts)
                    for nstats in this_stats:
                        for j, val in enumerate(nstats):
                            gvt_stats[j] += val
                    this_stats = gvt_stats

        return this_stats


def format_size(num, is_binary=True):
    div = 1024.0 if is_binary else 1000.0
    post_unit = "i" if is_binary else ""

    if num == 0:
        return "0"

    if abs(num) < div:
        if abs(num) > 1:
            return f"{num:3.1f}"
        num *= div
        for unit in ["m", "u", "n"]:
            if abs(num) > 1:
                return f"{num:3.1f}{unit}{post_unit}"
            num *= div
        return f"{num:3.1f}p{post_unit}"

    num /= div
    for unit in ["K", "M", "G", "T", "P"]:
        if abs(num) < div:
            return f"{num:.1f}{unit}{post_unit}"
        num /= div

    return f"{num:.1f}P{post_unit}"


def dump_text_report(filename):
    stats = RSStats(filename)

    simulation_time = stats.nodes_stats["node_total_time"][0] / 1000000
    hr_ticks_per_second = stats.nodes_stats["node_total_hr_time"][0] / simulation_time

    processed_msgs = stats.thread_metric_get("processed messages", aggregate_nodes=True, aggregate_gvts=True)
    anti_msgs = stats.thread_metric_get("anti messages", aggregate_nodes=True, aggregate_gvts=True)
    rollback_msgs = stats.thread_metric_get("rolled back messages", aggregate_nodes=True, aggregate_gvts=True)
    silent_msgs = stats.thread_metric_get("silent messages", aggregate_nodes=True, aggregate_gvts=True)
    rollbacks = stats.thread_metric_get("rollbacks", aggregate_nodes=True, aggregate_gvts=True)
    checkpoints = stats.thread_metric_get("checkpoints", aggregate_nodes=True, aggregate_gvts=True)
    msgs_cost = stats.thread_metric_get("processed messages time", aggregate_nodes=True, aggregate_gvts=True)
    checkpoints_cost = stats.thread_metric_get("checkpoints time", aggregate_nodes=True, aggregate_gvts=True)
    recoveries_cost = stats.thread_metric_get("recovery time", aggregate_nodes=True, aggregate_gvts=True)
    avg_log_size = stats.thread_metric_get("checkpoints size", aggregate_nodes=True, aggregate_gvts=True) / \
                   checkpoints if checkpoints != 0 else 0

    avg_msg_cost = 0 if processed_msgs == 0 else msgs_cost / (processed_msgs * hr_ticks_per_second)
    avg_checkpoint_cost = 0 if checkpoints == 0 else checkpoints_cost / (checkpoints * hr_ticks_per_second)
    avg_recovery_cost = 0 if rollbacks == 0 else recoveries_cost / (rollbacks * hr_ticks_per_second)

    rollback_freq = 100 * rollbacks / processed_msgs if processed_msgs != 0 else 0
    rollback_len = rollback_msgs / rollbacks if rollbacks != 0 else 0
    efficiency = 100 * (processed_msgs - rollback_msgs) / processed_msgs if processed_msgs else 100

    peak_memory_usage = sum(stats.nodes_stats["maximum_resident_set"])
    lps_count = sum(stats.nodes_stats["lps"])

    if len(stats.gvts) == 0:
        avg_memory_usage = 0.0
        sim_speed = 0.0
        last_gvt = 0.0
    else:
        avg_memory_usage = sum([sum(t) for t in stats.nodes_stats["resident_set"]]) / len(stats.gvts)
        last_gvt = stats.gvts[-1]
        sim_speed = last_gvt / len(stats.gvts)

    out_name = sys.argv[1][:-4] if sys.argv[1].endswith(".bin") else sys.argv[1]
    out_name = out_name + ".txt"

    with open(out_name, "w+", encoding="utf8") as f:
        f.write(f"TOTAL SIMULATION TIME ..... : {format_size(simulation_time, False)}s\n")
        f.write(f"TOTAL KERNELS ............. : {stats.nodes_count}\n")
        f.write(f"TOTAL_THREADS ............. : {sum(stats.threads_count)}\n")
        f.write(f"TOTAL_LPs ................. : {lps_count}\n")
        f.write(f"TOTAL EXECUTED EVENTS ..... : {processed_msgs + silent_msgs}\n")
        f.write(f"TOTAL COMMITTED EVENTS..... : {processed_msgs - rollback_msgs}\n")
        f.write(f"TOTAL REPROCESSED EVENTS... : {rollback_msgs}\n")
        f.write(f"TOTAL SILENT EVENTS........ : {silent_msgs}\n")
        f.write(f"TOTAL ROLLBACKS EXECUTED... : {rollbacks}\n")
        f.write(f"TOTAL ANTIMESSAGES......... : {anti_msgs}\n")
        f.write(f"ROLLBACK FREQUENCY......... : {rollback_freq:.2f}%\n")
        f.write(f"ROLLBACK LENGTH............ : {rollback_len:.2f}\n")
        f.write(f"EFFICIENCY................. : {efficiency:.2f}%\n")
        f.write(f"AVERAGE EVENT COST......... : {format_size(avg_msg_cost, False)}s\n")
        f.write(f"AVERAGE CHECKPOINT COST.... : {format_size(avg_checkpoint_cost, False)}s\n")
        f.write(f"AVERAGE RECOVERY COST...... : {format_size(avg_recovery_cost, False)}s\n")
        f.write(f"AVERAGE LOG SIZE........... : {format_size(avg_log_size)}B\n")
        f.write(f"LAST COMMITTED GVT ........ : {last_gvt}\n")
        f.write(f"NUMBER OF GVT REDUCTIONS... : {len(stats.gvts)}\n")
        f.write(f"SIMULATION TIME SPEED...... : {sim_speed}\n")
        f.write(f"AVERAGE MEMORY USAGE....... : {format_size(avg_memory_usage)}B\n")
        f.write(f"PEAK MEMORY USAGE.......... : {format_size(peak_memory_usage)}B\n")


# Produce a boring textual report
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Please, supply the name of the raw statistics file!", file=sys.stderr)
        sys.exit(1)

    dump_text_report(sys.argv[1])
