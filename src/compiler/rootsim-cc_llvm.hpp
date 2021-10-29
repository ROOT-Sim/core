/**
 * @file instr/instr_llvm.hpp
 *
 * @brief LLVM compiler plugin support for rootsim-cc
 *
 * This is the header of the LLVM plugin used to manipulate model's code.
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
