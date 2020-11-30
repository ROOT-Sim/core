#pragma once

struct instr_cfg {
	const char *proc_suffix;
	const char *sub_suffix;
	const char *const *to_substitute;
	const char *const *to_ignore;
};

extern struct instr_cfg instr_cfg;
