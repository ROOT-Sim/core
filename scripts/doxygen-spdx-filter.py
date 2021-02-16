#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
import re
import sys

website = "https://hpdcs.github.io"

with open(sys.argv[1], 'r') as f:
    file_text = f.read()

file_text = re.sub(r"SPDX-FileCopyrightText: 2008-([0-9]{4}) HPDCS Group .*>",
                   r"@copyright Copyright (C) 2008-\1 HPDCS Group " + website,
                           file_text)

# TODO: improve the spdx license substitution
file_text = re.sub(r"SPDX-License-Identifier:.*", "", file_text)

print(file_text)
