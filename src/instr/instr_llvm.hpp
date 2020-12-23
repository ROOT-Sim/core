/**
 * @file instr/instr_llvm.hpp
 *
 * @brief LLVM plugin to instrument memory writes
 *
 * This is the header of the LLVM plugin which instruments memory allocations so
 * as to enable transparent rollbacks of application code state.
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://hpdcs.github.io
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

struct instr_cfg {
	const char *proc_suffix;
	const char *sub_suffix;
	const char *const *to_substitute;
	const char *const *to_ignore;
};

extern struct instr_cfg instr_cfg;
