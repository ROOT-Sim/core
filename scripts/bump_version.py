#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
import os
import re
import sys
from subprocess import check_output

describe_str = check_output(['git', 'describe']).decode()
branch_name = check_output(['git', 'branch', '--show-current']).decode().strip()

# Split version in major, minor, hotfix, and tag
version = describe_str.split("-", 1)[0]
version = version[1:]  # This is to remove the 'v' at the beginning of the previous tag name
tag = "-" + describe_str.split("-", 1)[1].split("-", 1)[0]
major, minor, hotfix = map(int, version.split(".", 3))

new_tag = ""
if tag.startswith("-alpha") or tag.startswith("-beta") or tag.startswith("-rc"):
    tag_version = int(tag.split(".")[1])
    new_tag = tag.split(".")[0]
    new_tag = f"{new_tag}.{tag_version + 1}"

branch_type = branch_name.split("-", 1)[0]

print(f"The current branch {branch_name} is of type {branch_type}")
print(f"Current version is {major}.{minor}.{hotfix}{tag}")

if new_tag == "":
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

print(f"Applying new version number: {major}.{minor}.{hotfix}{new_tag}. Continue? (y/n)")
if sys.stdin.read(1) != 'y':
    sys.exit(0)

root_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
build_path = os.path.join(root_path, "CMakeLists.txt")

with open(build_path, "r") as f:
    cmakelists = f.read()

cmakelists, sub_cnt = re.subn(r"PROJECT_VERSION .*\)",
                              f"PROJECT_VERSION {major}.{minor}.{hotfix}{new_tag})",
                              cmakelists, count=1)

if sub_cnt != 1:
    print("Version pattern not found. The operation won't be completed.")
    sys.exit(1)

with open(build_path, "w") as f:
    f.write(cmakelists)

print(f"File modified successfully, version bumped to {major}.{minor}.{hotfix}{new_tag}")
