#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

"""This is a use example of the RSStats class
You can actually include this module in your code and use the object as it is done here"""
import sys

import matplotlib.pyplot as plt

from rootsim_stats import RSStats


def compute_diffs(values):
    """
    Compute the difference between each value and the previous one

    :parameter values: the list of values
    :return: the list of differences
    """
    ret = []
    iter_values = iter(values)
    ret.append(next(iter_values))
    for i, value in enumerate(iter_values):
        ret.append(value - values[i])
    return ret


def plot_data(data_label, rts, data, sub_fig):
    """
    Plot the data

    :parameter data_label: the label of the data
    :parameter rts: the list of real times
    :parameter data: the list of data
    """
    sub_fig.plot(rts, data, marker='.', markersize=3, markeredgecolor=(0.2, 0.3, 0.7), color=(0.4, 0.5, 0.7))
    sub_fig.set_xlabel('Real Time (in s)')
    sub_fig.set_ylabel(data_label)
    sub_fig.label_outer()
    sub_fig.grid(True)


def dump_tsv_data(data_label, rts, data):
    """
    Dump the data in a tsv file

    :parameter data_label: the label of the data
    :parameter rts: the list of real times
    :parameter data: the list of data
    """
    with open(data_label.replace(" ", "_").lower() + '.tsv', 'w', encoding="utf8") as f:
        f.write(f'Real time\t{data_label}\n')
        for i, sample in enumerate(data):
            f.write(f'{rts[i]}\t{sample}\n')


def plot_single(filename, dump_tsv=False):
    """
    Plot the data contained in the file

    :parameter filename: the name of the file
    :parameter dump_tsv: if True, dump the data in a tsv file, otherwise plot them
    """
    stats = RSStats(filename)
    rts = stats.rts(reduction=lambda x: sum(x) / len(x))
    rts = [value / 1000000 for value in rts]
    time_diff = compute_diffs(rts)
    gvt_advancement = compute_diffs(stats.gvts)
    gvt_advancement = [gvt_advancement[i] / time_diff[i] for i in range(len(time_diff))]

    proc_msgs = stats.thread_metric_get("processed messages", aggregate_threads=True, aggregate_nodes=True)
    rbs_msgs = stats.thread_metric_get("rolled back messages", aggregate_threads=True, aggregate_nodes=True)
    proc_msgs = [(proc_msgs[i] - rbs_msgs[i]) / time_diff[i] for i in range(len(time_diff))]

    rollbacks = stats.thread_metric_get("rollbacks", aggregate_threads=True, aggregate_nodes=True)
    rollbacks = [rollbacks[i] / time_diff[i] for i in range(len(time_diff))]

    if dump_tsv:
        dump_tsv_data("Committed events per second", rts, proc_msgs)
        dump_tsv_data("Rollbacks per second", rts, rollbacks)
        dump_tsv_data("GVT advancement per second", rts, gvt_advancement)
    else:
        plt.rcParams['font.family'] = ['monospace']
        plt.rcParams["axes.unicode_minus"] = False
        _, figxs = plt.subplots(3)
        plot_data("Committed events per second", rts, proc_msgs, figxs[0])
        plot_data("Rollbacks per second", rts, rollbacks, figxs[1])
        plot_data("GVT advancement per second", rts, gvt_advancement, figxs[2])
        plt.show()


def plot_single_main():
    """Main function to plot the data from a file"""
    tsv_arg = True
    try:
        sys.argv.remove('--tsv')
    except ValueError:
        tsv_arg = False

    if len(sys.argv) != 2:
        print("Please, supply the name of a raw statistics file!", file=sys.stderr)
        sys.exit(1)

    plot_single(sys.argv[1], tsv_arg)


if __name__ == "__main__":
    plot_single_main()
