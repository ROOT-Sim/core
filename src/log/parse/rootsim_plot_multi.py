#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2025 HPCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

"""This is a use example of the RSStats class
You can actually include this module in your code and use the object as it is done here"""
import sys

import matplotlib.pyplot as plt

from rootsim_stats import RSStats


def get_stats(file_name):
    """Get the statistics from a file and return the number of threads, the total time of the first node,
    the rollback frequency and the efficiency

    :parameter file_name: the name of the file to parse
    :return: a tuple with the total number of threads across all nodes, the total time of the first node, the rollback
    percentage w.r.t. to the total number of processed events and the efficiency expresses as a percentage
    """
    rs_stats = RSStats(file_name)
    processed_msgs = rs_stats.thread_metric_get("processed messages", aggregate_nodes=True, aggregate_gvts=True)
    rollback_msgs = rs_stats.thread_metric_get("rolled back messages", aggregate_nodes=True, aggregate_gvts=True)
    rollbacks = rs_stats.thread_metric_get("rollbacks", aggregate_nodes=True, aggregate_gvts=True)

    rollback_percentage = 100 * rollbacks / processed_msgs if processed_msgs != 0 else 0
    efficiency = 100 * (processed_msgs - rollback_msgs) / processed_msgs if processed_msgs else 100
    total_seconds_first_node = rs_stats.nodes_stats["processing_time"][0] / 1000000

    return sum(rs_stats.threads_count), total_seconds_first_node, rollback_percentage, efficiency


def compute_avg(values):
    """
    Compute the average of a list of values

    :parameter values: the list of values
    :return: the average of the values
    """
    return sum(values) / len(values)


def plot_data(data_label, threads, data, sub_fig):
    """
    Plot the data

    :parameter data_label: the label of the data
    :parameter threads: the list of threads
    :parameter data: the list of data
    :parameter sub_fig: the subplot to plot
    """
    sub_fig.set_xticks(threads)
    sub_fig.plot(threads, data, marker='.', markersize=3, markeredgecolor=(0.2, 0.3, 0.7), color=(0.4, 0.5, 0.7))
    sub_fig.set_xlabel('Threads')
    sub_fig.set_ylabel(data_label)
    sub_fig.label_outer()
    sub_fig.grid(True)


def dump_tsv_data(data_label, threads, data):
    """
    Dump the data in a tsv file

    :parameter data_label: the label of the data
    :parameter threads: the list of threads
    :parameter data: the list of data
    """
    with open(data_label.replace(" ", "_").lower() + '.tsv', 'w', encoding="utf8") as f:
        f.write(f'Threads\t{data_label}\n')
        for i, sample in enumerate(data):
            f.write(f'{threads[i]}\t{sample}\n')


def plot_multi(filenames, dump_tsv=False):
    """
    Plot the data from multiple files

    :parameter filenames: the list of files to parse
    :parameter dump_tsv: if True, dump the data in tsv files, otherwise plot the data
    """
    all_data = {}
    for filename in filenames:
        threads_count, total_time, rollback_freq, efficiency = get_stats(filename)
        if threads_count not in all_data:
            all_data[threads_count] = ([], [], [])
        all_data[threads_count][0].append(total_time)
        all_data[threads_count][1].append(rollback_freq)
        all_data[threads_count][2].append(efficiency)

    threads = sorted(all_data.keys())
    times = []
    rollback_probs = []
    efficiencies = []
    for thr in threads:
        times.append(compute_avg(all_data[thr][0]))
        rollback_probs.append(compute_avg(all_data[thr][1]))
        efficiencies.append(compute_avg(all_data[thr][2]))

    if dump_tsv:
        dump_tsv_data("Processing time (seconds)", threads, times)
        dump_tsv_data("Rollback (%)", threads, rollback_probs)
        dump_tsv_data("Efficiency (%)", threads, efficiencies)
    else:
        plt.rcParams['font.family'] = ['monospace']
        plt.rcParams["axes.unicode_minus"] = False
        _, figxs = plt.subplots(3)
        plot_data(r"Processing time ($\it{seconds}$)", threads, times, figxs[0])
        plot_data(r"Rollback ($\it{\%}$)", threads, rollback_probs, figxs[1])
        plot_data(r"Efficiency ($\it{\%}$)", threads, efficiencies, figxs[2])
        plt.show()


def plot_multi_main():
    """Main function to plot the data from multiple files"""
    tsv_arg = True
    try:
        sys.argv.remove('--tsv')
    except ValueError:
        tsv_arg = False

    if len(sys.argv) < 2:
        print("Please, supply the name of at least a raw statistics file!", file=sys.stderr)
        sys.exit(1)

    plot_multi(sys.argv[1:], tsv_arg)


if __name__ == "__main__":
    plot_multi_main()
