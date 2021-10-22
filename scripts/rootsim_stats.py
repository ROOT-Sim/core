import copy
import math
import struct

# TODO proper documentation of the RSStats object
# TODO expose memory statistics
# TODO provide also statistics normalized by wall clock time
class RSStats:
    _STATS_MAX_STRLEN = 32

    @staticmethod
    def _pattern_unpack(ptrn, data):
        big_endian, data = data
        ptrn_size = struct.calcsize(ptrn)
        ret = struct.unpack((">" if big_endian else "<") + ptrn, data[:ptrn_size])
        return ret, (big_endian, data[ptrn_size:])

    @staticmethod
    def _endian_check(data):
        data = (True, data)
        tup, data = RSStats._pattern_unpack("H", data)
        big_endian = None
        if tup[0] == 61455:
            big_endian = True
        elif tup[0] == 4080:
            big_endian = False
        return big_endian, data[1]

    @staticmethod
    def _names_unpack(data):
        tup, data = RSStats._pattern_unpack("q", data)
        n_stats = tup[0]
        ret = []
        for _ in range(n_stats):
            tup, data = RSStats._pattern_unpack(f"{RSStats._STATS_MAX_STRLEN}s", data)
            ret.append(tup[0].decode("utf-8").rstrip('\0'))
        return ret, data

    @staticmethod
    def _threads_unpack(data, t_metrics_len):
        tup, data = RSStats._pattern_unpack("q", data)
        t_stats_fmt = str(t_metrics_len) + "Q"
        n_stats = tup[0] // (t_metrics_len * 8)
        ret = []
        for i in range(n_stats):
            tup, data = RSStats._pattern_unpack(t_stats_fmt, data)
            ret.append(tup)
        return ret, data

    @staticmethod
    def _nodes_unpack(data, t_metrics_len):
        tup, data = RSStats._pattern_unpack("q", data)
        n_nodes = tup[0]
        ret = []
        for _ in range(n_nodes):
            glob_stats, data = RSStats._pattern_unpack("8Q", data)
            n_threads = glob_stats[0]
            tup, data = RSStats._pattern_unpack("q", data)
            n_stats = tup[0] // (2 * 8)
            node_stats = []
            for i in range(n_stats):
                tup, data = RSStats._pattern_unpack("dQ", data)
                node_stats.append(tup)

            threads_stats = []
            for i in range(n_threads):
                tup, data = RSStats._threads_unpack(data, t_metrics_len)
                threads_stats.append(tup)

            ret.append((glob_stats, node_stats, threads_stats))

        return ret, data

    @staticmethod
    def _truncate_to_last_gvt(stats):
        min_gvts = len(stats[0][1])
        for _, node_stats, threads_stats in stats:
            min_gvts = min(len(node_stats), min_gvts)
            for t_stats in threads_stats:
                min_gvts = min(len(t_stats), min_gvts)

        ret = []
        for glob_stats, node_stats, threads_stats in stats:
            t_node_stats = node_stats[:min_gvts]
            t_threads_stats = []
            for t_stats in threads_stats:
                t_threads_stats.append(t_stats[:min_gvts])
            ret.append((glob_stats, t_node_stats, t_threads_stats))

        return ret, min_gvts

    def __init__(self, rs_stats_file):
        with open(rs_stats_file, 'rb') as f:
            data = f.read()

        data = self._endian_check(data)
        metric_names, data = self._names_unpack(data)
        metric_cnt = len(metric_names)
        all_stats, data = self._nodes_unpack(data, metric_cnt)

        self._gvts = None

        if len(data[1]) != 0:
            raise RuntimeWarning("Parsing failed, there's garbage at the end")

        self._metrics = {}
        for metric_name in metric_names:
            self._metrics[metric_name] = []

        all_stats, gvts_cnt = self._truncate_to_last_gvt(all_stats)

        for i, triple in enumerate(all_stats):
            glob_stats, node_stats, threads_stats = triple
            if self._gvts is None:
                self._gvts = []
                for gvt, crs_mem in node_stats:
                    self._gvts.append(gvt)
            # else TODO sanity check that stuff is equal

            for j, metric_name in enumerate(metric_names):
                metric_n_stat = []
                for t_stat in threads_stats:
                    metric_t_stat = []
                    for tup in t_stat:
                        metric_t_stat.append(tup[j])
                    metric_n_stat.append(metric_t_stat)

                self._metrics[metric_name].append(metric_n_stat)

    @property
    def gvts(self):
        return list(self._gvts)

    @property
    def metrics(self):
        return list(self._metrics)

    def thread_metric_get(self, metric, aggregate_gvts=False,
                          aggregate_resources=False, aggregate_nodes=False):
        if metric not in self._metrics:
            raise RuntimeError(f"Asked stats for the non-existing thread_metric {metric}")

        if aggregate_nodes:
            aggregate_resources = True

        this_stats = copy.deepcopy(self._metrics[metric])

        if aggregate_gvts:
            for nstats in this_stats:
                for i in range(len(nstats)):
                    nstats[i] = sum(nstats[i])

            if aggregate_resources:
                for i in range(len(this_stats)):
                    this_stats[i] = sum(this_stats[i])

                if aggregate_nodes:
                    this_stats = sum(this_stats)
        else:
            if aggregate_resources:
                for i in range(len(this_stats)):
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
