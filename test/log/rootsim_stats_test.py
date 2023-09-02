#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

"""
This script contains tests for the rootsim_stats.py script. It tests whether the output of rootsim_stats.py matches
the expected results for several simulated scenarios. The tests check the correctness of the regular expressions
used to parse the output of the rootsim_stats.py script, as well as the correctness of the statistics values computed
by the script.

The script requires two command line arguments: the path to the rootsim_stats.py script, and the path to the test
folder containing the statistics binaries. The test_stats_file() function tests a single statistics file, while the
main function calls test_stats_file() for each test case and checks the exit value to determine whether the tests
passed or failed.
"""
import os
import re
import runpy
import sys
import textwrap


def regex_get():
    """
    Get the regex for the stats

    :return: the regex for the stats
    """
    count_regex = r"(\d+)"
    float_regex = r"(\d+(?:\.\d+)?)"
    measure_regex = f"{float_regex}(?:[munKMGTPEZ]i?)?"

    stats_regex_str = f'''\
        TOTAL SIMULATION TIME ..... : {measure_regex}s
        TOTAL PROCESSING TIME ..... : {measure_regex}s
        TOTAL KERNELS ............. : {count_regex}
        TOTAL THREADS ............. : {count_regex}
        TOTAL LPS ................. : {count_regex}
        TOTAL EXECUTED EVENTS ..... : {count_regex}
        TOTAL COMMITTED EVENTS..... : {count_regex}
        TOTAL REPROCESSED EVENTS... : {count_regex}
        TOTAL SILENT EVENTS........ : {count_regex}
        TOTAL ROLLBACKS EXECUTED... : {count_regex}
        TOTAL ANTIMESSAGES......... : {count_regex}
        ROLLBACK FREQUENCY......... : {float_regex}%
        ROLLBACK LENGTH............ : {float_regex}
        EFFICIENCY................. : {float_regex}%
        AVERAGE EVENT COST......... : {measure_regex}s
        AVERAGE CHECKPOINT COST.... : {measure_regex}s
        AVERAGE RECOVERY COST...... : {measure_regex}s
        AVERAGE CHECKPOINT SIZE.... : {measure_regex}B
        LAST COMMITTED GVT ........ : {float_regex}
        NUMBER OF GVT REDUCTIONS... : {count_regex}
        SIMULATION TIME SPEED...... : {float_regex}
        AVERAGE MEMORY USAGE....... : {measure_regex}B
        PEAK MEMORY USAGE.......... : {measure_regex}B
        '''

    stats_regex_str = textwrap.dedent(stats_regex_str)
    return re.compile(stats_regex_str, re.MULTILINE)


def test_init():
    """
    Initialize the test

    :return: the script path and the test folder
    """
    if len(sys.argv) < 3:
        raise RuntimeError("Need the rootsim_stats.py path and the test folder containing the statistics binaries!")

    script_path = sys.argv[1]
    test_folder = sys.argv[2]
    del sys.argv[2:]
    return script_path, test_folder


def test_stats_file(base_name, expected):
    """
    Test the stats file

    :param base_name: the base name of the stats file
    :param expected: the expected stats
    """
    full_base_name = os.path.join(BIN_FOLDER, base_name)
    if not os.path.isfile(full_base_name + ".bin"):
        sys.exit(1)
    sys.argv[1] = full_base_name + ".bin"
    runpy.run_path(path_name=RS_SCRIPT_PATH, run_name="__main__")
    with open(full_base_name + ".txt", "r", encoding="utf8") as report_file:
        data = report_file.read()

    match = STATS_REGEX.fullmatch(data)
    if match is None:
        sys.exit(1)

    for i, expected_field in enumerate(expected):
        if expected_field == 'NZ':
            if float(match[i + 1]) == 0:
                print('NZ', match[i + 1])
                sys.exit(1)
        elif expected_field != match[i + 1]:
            print(expected_field, match[i + 1])
            sys.exit(1)


if __name__ == "__main__":
    RS_SCRIPT_PATH, BIN_FOLDER = test_init()
    STATS_REGEX = regex_get()
    test_stats_file("empty_stats", ["NZ", "0", "1", "2", "0", "0", "0", "0", "0", "0", "0", "0.00", "0.00", "100.00",
                                    "0", "0", "0", "0", "0.0", "0", "0.0", "0", "NZ"])
    test_stats_file("single_gvt_stats", ["NZ", "0", "1", "2", "16", "0", "0", "0", "0", "0", "0", "0.00", "0.00",
                                         "100.00", "0", "0", "0", "0", "0.0", "1", "0.0", "NZ", "NZ"])
    test_stats_file("multi_gvt_stats", ["NZ", "0", "1", "2", "16", "0", "0", "0", "0", "0", "0", "0.00", "0.00",
                                        "100.00", "0", "0", "0", "0", "48.56", "4", "12.14", "NZ", "NZ"])
    test_stats_file("measures_stats", ["NZ", "0", "1", "2", "16", "156", "102", "24", "30", "20", "60", "15.87", "1.20",
                                       "80.95", "0", "0", "0", "0", "0.0", "1", "0.0", "NZ", "NZ"])

    # TODO: test the actual RSStats python object
