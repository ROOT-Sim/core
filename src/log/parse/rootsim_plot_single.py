#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

import matplotlib.pyplot as plt
from rootsim_stats import RSStats
import sys

# This is a use example of the RSStats class
# You can actually include this module in your code and use the object as it is done here
if __name__ == "__main__":

    dump_tsv = True
    try:
        sys.argv.remove('--tsv')
    except ValueError:
        dump_tsv = False

    if len(sys.argv) != 2:
        print("Please, supply the name of a raw statistics file!", file=sys.stderr)
        exit(-1)

    plt.rcParams['font.family'] = ['monospace']
    plt.rcParams["axes.unicode_minus"] = False

    def compute_diffs(d):
        ret = []
        iter_d = iter(d)
        ret.append(next(iter_d))
        for i, v in enumerate(iter_d):
            ret.append(v - d[i])
        return ret

    stats = RSStats(sys.argv[1])
    rts = stats.rts(reduction=lambda x: sum(x) / len(x))
    rts = [v / 1000000 for v in rts]
    time_diff = compute_diffs(rts)
    gvt_advancement = compute_diffs(stats.gvts)
    gvt_advancement = [gvt_advancement[i] / time_diff[i] for i in range(len(time_diff))]

    _, figxs = plt.subplots(3)

    def plot_data(sub_fig, data, data_label):
        if dump_tsv:
            with open(data_label.replace(" ", "_").lower() + '.tsv', 'w') as f:
                f.write(f'Real time\t{data_label}\n')
                for i, sample in enumerate(data):
                    f.write(f'{rts[i]}\t{sample}\n')
        else:
            sub_fig.plot(rts, data, marker='.', markersize=3, markeredgecolor=(0.2, 0.3, 0.7), color=(0.4, 0.5, 0.7))
            sub_fig.set_xlabel('Real Time (in s)')
            sub_fig.set_ylabel(data_label)
            sub_fig.label_outer()
            sub_fig.grid(True)

    def plot_metric(sub_fig, metric_name, metric_display):
        metrics = stats.thread_metric_get(metric_name, aggregate_threads=True, aggregate_nodes=True)
        metrics = [metrics[i] / time_diff[i] for i in range(len(time_diff))]

        plot_data(sub_fig, metrics, metric_display)

    plot_metric(figxs[0], "processed messages", "Processed messages")
    plot_metric(figxs[1], "rollbacks", "Rollbacks")
    plot_data(figxs[2], gvt_advancement, "GVT advancement")

    if not dump_tsv:
        plt.show()
