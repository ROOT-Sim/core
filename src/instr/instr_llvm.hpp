/**
 * @file instr/instr_llvm.hpp
 *
 * @brief LLVM plugin to instrument memory writes
 *
 * This is the header of the LLVM plugin which instruments memory allocations so
 * as to enable transparent rollbacks of application code state.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

struct instr_cfg {
	const char *proc_suffix;
	const char *sub_suffix;
	const char *const *to_substitute;
	const char *const *to_ignore;
};

extern struct instr_cfg instr_cfg;
