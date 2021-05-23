#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
import re
from subprocess import check_output
import sys
import os

describe_str = check_output(['git', 'describe']).decode()
branch_name = check_output(['git', 'branch', '--show-current']).decode().strip()

# Split version in major, minor, hotfix
version = describe_str.split("-", 1)[0]
major, minor, hotfix = map(int, version.split(".", 3))

branch_type = branch_name.split("-", 1)[0]

print(f"The current branch {branch_name} is of type {branch_type}")
print(f"Current version is {major}.{minor}.{hotfix}")

# Check if we have to increment the major
if len(sys.argv) > 1 and sys.argv[1] ==  "major":
    major += 1
    minor = 0
    hotfix = 0
elif branch_type == "release":
    minor += 1
    hotfix = 0
elif branch_type == "hotfix":
    hotfix += 1
else:
    print("Branch name doesn't tell the version should be bumped.")
    sys.exit(1)

print(f"Applying new version number: {major}.{minor}.{hotfix}. Continue? (y/n)")
if sys.stdin.read(1) != 'y':
    sys.exit(0)

root_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
meson_build_path = os.path.join(root_path, "meson.build")

with open(meson_build_path, "r") as f:
    meson_build = f.read()

meson_build, sub_cnt = re.subn(r"version\s*:\s*'.*'",
                               f"version : '{major}.{minor}.{hotfix}'",
                               meson_build, count=1)

if sub_cnt != 1:
    print("Version pattern not found. The operation won't be completed.")
    sys.exit(1)

with open(meson_build_path, "w") as f:
    f.write(meson_build)

print(f"File modified successfully, version bumped to {major}.{minor}.{hotfix}")
