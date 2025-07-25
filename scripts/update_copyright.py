#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

"""Update the year in the license header of all files in the repository"""
import datetime
import os
import re

year = datetime.datetime.now().year
ROOT_PATH = os.getcwd()

for root, _, files in os.walk(ROOT_PATH):
    for file_name in files:
        if not (file_name.endswith(".py") or file_name.endswith(".c") or
                file_name.endswith(".cpp") or file_name.endswith(".h") or
                file_name.endswith(".hpp") or file_name.endswith(".md") or
                file_name.endswith(".txt") or file_name.startswith(".")):
            continue

        file_path = os.path.join(root, file_name)
        print("Updatigng copyright in ", file_path)
        with open(file_path, 'r', encoding='utf-8') as f:
            file_text = f.read()

        file_text = re.sub(r"2008-[0-9]{4} HPDCS Group",
                           f"2008-{year} HPDCS Group", file_text)

        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(file_text)
