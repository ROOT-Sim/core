#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
import sys
import subprocess as sp

model_sp = sp.Popen(sys.argv[1:], stdin=sp.DEVNULL, stdout=sp.PIPE, stderr=sp.PIPE)
out, errs = model_sp.communicate()
sys.exit(model_sp.returncode)
