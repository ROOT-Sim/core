#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

import sys
import matplotlib.pyplot as plt
from rootsim_stats import RSStats


def compute_diffs(values):
    ret = []
    iter_values = iter(values)
    ret.append(next(iter_values))
    for i, val in enumerate(iter_values):
        ret.append(val - values[i])
    return ret


def plot_data(data_label, rts, data, sub_fig):
    sub_fig.plot(rts, data, marker='.', markersize=3, markeredgecolor=(0.2, 0.3, 0.7), color=(0.4, 0.5, 0.7))
    sub_fig.set_xlabel('Real Time (in s)')
    sub_fig.set_ylabel(data_label)
    sub_fig.label_outer()
    sub_fig.grid(True)


def dump_tsv_data(data_label, rts, data):
    with open(data_label.replace(" ", "_").lower() + '.tsv', 'w', encoding="utf8") as f:
        f.write(f'Real time\t{data_label}\n')
        for i, sample in enumerate(data):
            f.write(f'{rts[i]}\t{sample}\n')


def plot_single(filename, dump_tsv=False):
    plt.rcParams['font.family'] = ['monospace']
    plt.rcParams["axes.unicode_minus"] = False

    stats = RSStats(filename)
    rts = stats.rts(reduction=lambda x: sum(x) / len(x))
    rts = [val / 1000000 for val in rts]
    time_diff = compute_diffs(rts)
    gvt_advancement = compute_diffs(stats.gvts)
    gvt_advancement = [gvt_advancement[i] / time_diff[i] for i in range(len(time_diff))]

    proc_msgs = stats.thread_metric_get("processed messages", aggregate_threads=True, aggregate_nodes=True)
    proc_msgs = [proc_msgs[i] / time_diff[i] for i in range(len(time_diff))]

    rollbacks = stats.thread_metric_get("rollbacks", aggregate_threads=True, aggregate_nodes=True)
    rollbacks = [rollbacks[i] / time_diff[i] for i in range(len(time_diff))]

    if dump_tsv:
        dump_tsv_data("Processed messages", rts, proc_msgs)
        dump_tsv_data("Rollbacks", rts, rollbacks)
        dump_tsv_data("GVT advancement", rts, gvt_advancement)
    else:
        _, figxs = plt.subplots(3)
        plot_data("Processed messages", rts, proc_msgs, figxs[0])
        plot_data("Rollbacks", rts, rollbacks, figxs[1])
        plot_data("GVT advancement", rts, gvt_advancement, figxs[2])
        plt.show()


# This is a use example of the RSStats class
# You can actually include this module in your code and use the object as it is done here
if __name__ == "__main__":
    tsv_arg = True
    try:
        sys.argv.remove('--tsv')
    except ValueError:
        tsv_arg = False

    if len(sys.argv) != 2:
        print("Please, supply the name of a raw statistics file!", file=sys.stderr)
        sys.exit(1)

    plot_single(sys.argv[1], tsv_arg)
