#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

import matplotlib.pyplot as plt
from rootsim_stats import RSStats
import sys

# This is a use example of the RSStats class
# You can actually include this module in your code and use the object as it is done here
if __name__ == "__main__":

    if len(sys.argv) != 2:
        print("Please, supply the name of a raw statistics file!", file=sys.stderr)
        exit(-1)
    plt.rcParams['font.family'] = ['monospace']
    plt.rcParams["axes.unicode_minus"] = False

    def compute_avg(l):
        return sum(l) / len(l)

    def compute_diffs(d):
        ret = []
        for i, v in enumerate(d):
            if i == 0:
                ret.append(v)
            else:
                ret.append(d[i] - d[i - 1])
        return ret

    stats = RSStats(sys.argv[1])
    rts = stats.rts(reduction=compute_avg)
    rts = [v / 1000000 for v in rts]
    time_diff = compute_diffs(rts)

    _, figxs = plt.subplots(3)

    def plot_data(sub_fig, data, data_label):
        sub_fig.plot(rts, data, marker='.', markersize=3, markeredgecolor=(0.2, 0.3, 0.7), color=(0.4, 0.5, 0.7))
        sub_fig.set_xlabel('Real Time (in s)')
        sub_fig.set_ylabel(data_label)
        sub_fig.label_outer()

        sub_fig.grid(True)

    def plot_metric(sub_fig, metric_name, metric_display):
        metrics = stats.thread_metric_get(metric_name, aggregate_threads=True, aggregate_nodes=True)
        for i, o in enumerate(time_diff):
            metrics[i] /= o

        plot_data(sub_fig, metrics, metric_display)

    plot_metric(figxs[0], "processed messages", "Processed messages")
    plot_metric(figxs[1], "rollbacks", "Rollbacks")

    gvt_advancement = compute_diffs(stats.gvts)
    for i, o in enumerate(time_diff):
        gvt_advancement[i] /= o
    plot_data(figxs[2], gvt_advancement, "GVT advancement")

    plt.show()
