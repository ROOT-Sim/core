#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
import os
import re
import datetime

year = datetime.datetime.now().year
root_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

for root, _, files in os.walk(root_path):
    for file_name in files:
        if not (file_name.endswith(".py") or file_name.endswith(".c") or
            file_name.endswith(".cpp") or file_name.endswith(".h") or
            file_name.endswith(".hpp") or file_name.endswith(".md") or
            file_name.endswith(".build")):
            continue

        file_path = os.path.join(root, file_name)
        with open(file_path, 'r') as f:
            file_text = f.read()

        file_text = re.sub(r"2008-[0-9]{4} HPDCS Group",
                           f"2008-{year} HPDCS Group", file_text)

        with open(file_path, 'w') as f:
            f.write(file_text)
