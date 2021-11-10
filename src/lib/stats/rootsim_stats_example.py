#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

import matplotlib.pyplot as plt
from rootsim_stats import RSStats
import sys

# This is a use example of the RSStats class
# You can actually include this module in your code and use the object as it is done here
if __name__ == "__main__":

    if len(sys.argv) != 2:
        print("Please, supply the name of the raw statistics file!", file=sys.stderr)
        exit(-1)

    stats = RSStats(sys.argv[1])
    gvts = stats.gvts
    fig, figxs = plt.subplots(4)

    def plot_metric(sub_fig, metric_name, metric_display):
        metrics = stats.thread_metric_get(metric_name,
                                          aggregate_resources=False,
                                          aggregate_nodes=False)
        labels = ["Thread " + str(i + 1) for i in range(len(metrics[0]))]
        sub_fig.stackplot(gvts, metrics[0], labels=labels)
        sub_fig.set_xlabel('GVT')
        sub_fig.set_ylabel(metric_display)
        sub_fig.label_outer()

        for xc in gvts:
            sub_fig.axvline(x=xc, color='grey', linestyle='-', linewidth=0.1)

    plot_metric(figxs[0], "processed messages", "Processed messages")
    plot_metric(figxs[1], "rollbacked messages", "Rollbacked messages")
    plot_metric(figxs[2], "silent messages", "Silent messages")
    plot_metric(figxs[3], "checkpoints", "Checkpoints")

    handles, labels = figxs[3].get_legend_handles_labels()
    fig.legend(handles, labels, loc='upper right')

    plt.show()
