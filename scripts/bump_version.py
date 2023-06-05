#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

"""
This module defines a script for updating the version number of a software project based on Git branch and tags.

The script uses the Git command-line interface to obtain the project version number and the name of the current Git
branch. It then extracts the major, minor, hotfix, and tag components of the version number and increments them
according to the branch type or the command-line arguments.

When executed, the script prompts the user to confirm the version update and modifies the "CMakeLists.txt" file in
the project root directory to reflect the new version number. If the version pattern is not found in the file, the
script terminates with an error message.
"""
import os
import re
import sys
from subprocess import check_output  # nosec B404

DESCRIBE_STR = check_output(['/usr/bin/git', 'describe'], shell=False).decode()
BRANCH_NAME = check_output(['/usr/bin/git', 'branch', '--show-current'], shell=False).decode().strip()

# Split version in major, minor, hotfix, and tag
version = DESCRIBE_STR.split("-", 1)[0]
tag = "-" + DESCRIBE_STR.split("-", 1)[1].split("-", 1)[0]
major, minor, hotfix = map(int, version.split(".", 3))

new_tag = ""
if tag.startswith("-alpha") or tag.startswith("-beta") or tag.startswith("-rc"):
    tag_version = int(tag.split(".")[1])
    new_tag = tag.split(".")[0]
    new_tag = f"{new_tag}.{tag_version + 1}"

branch_type = BRANCH_NAME.split("-", 1)[0]

print(f"The current branch {BRANCH_NAME} is of type {branch_type}")
print(f"Current version is {major}.{minor}.{hotfix}{tag}")

if new_tag == "":
    # Check if we have to increment the major
    if len(sys.argv) > 1 and sys.argv[1] == "major":
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

with open(build_path, "r", encoding='utf-8') as f:
    CMAKELISTS = f.read()

CMAKELISTS, sub_cnt = re.subn(r"PROJECT_VERSION .*\)",
                              f"PROJECT_VERSION {major}.{minor}.{hotfix}{new_tag})",
                              CMAKELISTS, count=1)

if sub_cnt != 1:
    print("Version pattern not found. The operation won't be completed.")
    sys.exit(1)

with open(build_path, "w", encoding='utf-8') as f:
    f.write(CMAKELISTS)

print(f"File modified successfully, version bumped to {major}.{minor}.{hotfix}{new_tag}")
