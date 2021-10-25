import copy
import struct

# TODO proper documentation of the RSStats object
# TODO expose memory statistics
# TODO provide also statistics normalized by wall clock time
class RSStats:
    _STATS_MAX_STRLEN = 32

    def _pattern_unpack(self, ptrn):
        ptrn_size = struct.calcsize(ptrn)
        data_parse = self.data[:ptrn_size]
        ret = struct.unpack((">" if self.big_endian else "<") + ptrn, data_parse)
        self.data = self.data[ptrn_size:]
        return ret

    def _metric_names_load(self):
        n_stats = self._pattern_unpack("q")[0]
        for _ in range(n_stats):
            raw_name = self._pattern_unpack(f"{RSStats._STATS_MAX_STRLEN}s")[0]
            self.metric_names.append(raw_name.decode("utf-8").rstrip('\0'))

    def _threads_unpack(self):
        metrics_len = len(self.metric_names)
        n_stats = self._pattern_unpack("q")[0] // (metrics_len * 8)
        t_stats_fmt = str(metrics_len) + "Q"
        ret = []
        for _ in range(n_stats):
            ret.append(self._pattern_unpack(t_stats_fmt))
        return ret

    def _nodes_stats_load(self):
        n_nodes = self._pattern_unpack("q")[0]
        for _ in range(n_nodes):
            glob_stats = self._pattern_unpack("8Q")
            n_threads = glob_stats[0]
            n_stats = self._pattern_unpack("q")[0] // 16
            node_stats = []
            for i in range(n_stats):
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

    def __init__(self, rs_stats_file):
        with open(rs_stats_file, 'rb') as f:
            self.data = f.read()

        self.big_endian = True
        magic_number = self._pattern_unpack("H")[0]
        if magic_number == 61455 or magic_number == 4080:
            self.big_endian = magic_number == 61455
        else:
            raise RuntimeWarning("Parsing failed, wrong initial magic number")

        self.metric_names = []
        self._metric_names_load()
        
        self.all_stats = []
        self._nodes_stats_load()

        if len(self.data) != 0:
            raise RuntimeWarning("Parsing failed, there's garbage at the end")

        self._gvts = []
        self._truncate_to_last_gvt()

        self._metrics = {}
        for metric_name in self.metric_names:
            self._metrics[metric_name] = []

        for i, triple in enumerate(self.all_stats):
            glob_stats, node_stats, threads_stats = triple
            if len(self._gvts) == 0:
                for gvt, crs_mem in node_stats:
                    self._gvts.append(gvt)
            # else TODO sanity check that stuff is equal

            for j, metric_name in enumerate(self.metric_names):
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
                for i, _ in enumerate(nstats):
                    nstats[i] = sum(nstats[i])

            if aggregate_resources:
                for i, _ in enumerate(this_stats):
                    this_stats[i] = sum(this_stats[i])

                if aggregate_nodes:
                    this_stats = sum(this_stats)
        else:
            if aggregate_resources:
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
