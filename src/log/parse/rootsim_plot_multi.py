#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

import sys
import matplotlib.pyplot as plt
from rootsim_stats import RSStats


def get_stats(file_name):
    rs_stats = RSStats(file_name)
    processed_msgs = rs_stats.thread_metric_get("processed messages", aggregate_nodes=True, aggregate_gvts=True)
    rollback_msgs = rs_stats.thread_metric_get("rolled back messages", aggregate_nodes=True, aggregate_gvts=True)
    rollbacks = rs_stats.thread_metric_get("rollbacks", aggregate_nodes=True, aggregate_gvts=True)

    rollback_freq = 100 * rollbacks / processed_msgs if processed_msgs != 0 else 0
    efficiency = 100 * (processed_msgs - rollback_msgs) / processed_msgs if processed_msgs else 100
    total_seconds_first_node = rs_stats.nodes_stats["node_total_time"][0] / 1000000

    return int(sum(rs_stats.threads_count)), total_seconds_first_node / 1000000, rollback_freq, efficiency


def compute_avg(values):
    return sum(values) / len(values)


def plot_data(data_label, threads, data, sub_fig):
    sub_fig.set_xticks(threads)
    sub_fig.plot(threads, data, marker='.', markersize=3, markeredgecolor=(0.2, 0.3, 0.7), color=(0.4, 0.5, 0.7))
    sub_fig.set_xlabel('Threads')
    sub_fig.set_ylabel(data_label)
    sub_fig.label_outer()
    sub_fig.grid(True)


def dump_tsv_data(data_label, threads, data):
    with open(data_label.replace(" ", "_").lower() + '.tsv', 'w', encoding="utf8") as f:
        f.write(f'Threads\t{data_label}\n')
        for i, sample in enumerate(data):
            f.write(f'{threads[i]}\t{sample}\n')


def plot_multi(filenames, dump_tsv=False):
    all_data = {}
    for filename in filenames:
        thr_cnt, t, rf, eff = get_stats(filename)
        if thr_cnt not in all_data:
            all_data[thr_cnt] = ([], [], [])
        all_data[thr_cnt][0].append(t)
        all_data[thr_cnt][1].append(rf)
        all_data[thr_cnt][2].append(eff)

    threads = sorted(all_data.keys())
    times = []
    rollback_probs = []
    efficiencies = []
    for thr in threads:
        times.append(compute_avg(all_data[thr][0]))
        rollback_probs.append(compute_avg(all_data[thr][1]))
        efficiencies.append(compute_avg(all_data[thr][2]))

    if dump_tsv:
        dump_tsv_data("Execution time", threads, times)
        dump_tsv_data("% Rollback", threads, rollback_probs)
        dump_tsv_data("% Efficiency", threads, efficiencies)
    else:
        plt.rcParams['font.family'] = ['mono']
        plt.rcParams["axes.unicode_minus"] = False
        fig, figxs = plt.subplots(3)
        plot_data("Execution time", threads, times, figxs[0])
        plot_data("% Rollback", threads, rollback_probs, figxs[1])
        plot_data("% Efficiency", threads, efficiencies, figxs[2])
        plt.show()


# This is a use example of the RSStats class
# You can actually include this module in your code and use the object as it is done here
if __name__ == "__main__":
    tsv_arg = True
    try:
        sys.argv.remove('--tsv')
    except ValueError:
        tsv_arg = False

    if len(sys.argv) < 2:
        print("Please, supply the name of at least a raw statistics file!", file=sys.stderr)
        sys.exit(1)

    plot_multi(sys.argv[1:], tsv_arg)
