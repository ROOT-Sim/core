#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
import argparse
import coverxygen
import os
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-b", "--build_dir", type=str, default="build",
                    help="choose the build folder where to find the doc")
parser.add_argument("-g", "--github", action='store_true',
                    help="format the output as a GitHub comment for CI")
args = parser.parse_args()

coverage_target = 60.0

root_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
build_dir = args.build_dir
xml_dir = f"{build_dir}/docs/xml/"
src_dir = root_path
out_file = f"{build_dir}/coverxygen_output"
covscope = "public,protected,private".split(",")
covkind = ("enum,enumvalue,friend,typedef,variable,function,signal,slot,class,"
           "struct,union,define,file,namespace,page").split(",")
covexclude = [f"{root_path}/README.md"]

cov_obj = coverxygen.Coverxygen(xml_dir, out_file, covscope, covkind, "summary",
                                src_dir, None, False, covexclude, [])
try:
    cov_obj.process()
except RuntimeError as l_error:
    sys.stderr.write("error: %s\n" % str(l_error))
    sys.exit(1)

with open(out_file, "r") as f:
    summary_content = f.read()

total_line = summary_content[summary_content.find("Total"):]
doc_cov_match = next(re.finditer(r"\d*\.\d+|\d+", total_line))
acceptable = float(doc_cov_match[0]) >= coverage_target

if args.github:
    icon = ":+1" if acceptable else ":exclamation:"
    acceptable_val = 1 if acceptable else 0
    message = "MESSAGE<<EOF\n"
    message += f"\"Documentation coverage is **{doc_cov_match[0]}%** {icon}\\n"
    message += "```\\n"
    message += summary_content.replace("\n", "\\n")
    message += "```\\n\"\n"
    message += "EOF\n"
    message += f"acceptable={acceptable_val}\n"
    with open(os.environ['GITHUB_ENV'], "w") as f:
        f.write(message)
else:
    acceptable_str = "" if acceptable else "not "
    print(summary_content)
    print(f"Documentation coverage is {acceptable_str}acceptable "
          f"(target: {coverage_target}%)")

