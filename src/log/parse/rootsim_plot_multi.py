#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

import matplotlib.pyplot as plt
from rootsim_stats import RSStats
import sys


def get_stats(file_name):
    rs_stats = RSStats(file_name)
    processed_msgs = rs_stats.thread_metric_get("processed messages", aggregate_nodes=True, aggregate_gvts=True)
    rollback_msgs = rs_stats.thread_metric_get("rolled back messages", aggregate_nodes=True, aggregate_gvts=True)
    rollbacks = rs_stats.thread_metric_get("rollbacks", aggregate_nodes=True, aggregate_gvts=True)

    rollback_freq = 100 * rollbacks / processed_msgs if processed_msgs != 0 else 0
    efficiency = 100 * (processed_msgs - rollback_msgs) / processed_msgs if processed_msgs else 100

    return int(sum(rs_stats.threads_count)), rs_stats.nodes_stats["node_total_time"][0] / 1000000, rollback_freq, efficiency


# This is a use example of the RSStats class
# You can actually include this module in your code and use the object as it is done here
if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Please, supply the name of at least a raw statistics file!", file=sys.stderr)
        exit(-1)

    plt.rcParams['font.family'] = ['monospace']
    plt.rcParams["axes.unicode_minus"] = False

    def compute_avg(l):
        return sum(l) / len(l)

    data = {}
    for s in sys.argv[1:]:
        thr_cnt, t, rf, eff = get_stats(s)
        if thr_cnt not in data:
            data[thr_cnt] = ([], [], [])
        data[thr_cnt][0].append(t)
        data[thr_cnt][1].append(rf)
        data[thr_cnt][2].append(eff)

    threads = sorted(data.keys())
    times = []
    rollback_probs = []
    efficiencies = []
    for thr in threads:
        times.append(compute_avg(data[thr][0]))
        rollback_probs.append(compute_avg(data[thr][1]))
        efficiencies.append(compute_avg(data[thr][2]))

    fig, figxs = plt.subplots(3)

    def plot_data(sub_fig, data, data_label):
        sub_fig.set_xticks(threads)
        sub_fig.plot(threads, data, marker='.', markersize=3, markeredgecolor=(0.2, 0.3, 0.7), color=(0.4, 0.5, 0.7))
        sub_fig.set_xlabel('Threads')
        sub_fig.set_ylabel(data_label)
        sub_fig.label_outer()

        sub_fig.grid(True)

    plt.rcParams['font.family'] = ['mono']

    plot_data(figxs[0], times, "Execution time")
    plot_data(figxs[1], rollback_probs, "% Rollback")
    plot_data(figxs[2], efficiencies, "% Efficiency")

    plt.show()
