# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

instr_cfg = custom_target(
  'instr_cfg',
  output : 'instr_cfg.cpp',
  input : [],
  command : [llvm_cfg_gen, '@OUTPUT@']
)

rootsim_llvm_lib_dep = [llvm_dep]
if get_option('b_sanitize') == 'address'
  rootsim_llvm_lib_dep += meson.get_compiler('c').find_library('asan',
    static : true)
endif

rootsim_llvm_lib_cpp_args = [log_level_def, '-Wno-unused-parameter']

# ROOT-Sim LLVM plugin for model instrumentation
rootsim_llvm_lib = shared_library('rootsim-llvm', ['instr_llvm.cpp', instr_cfg],
  dependencies : rootsim_llvm_lib_dep, name_prefix : 'lib', name_suffix : 'so',
  cpp_args : rootsim_llvm_lib_cpp_args, include_directories : src_inc_dir,
  install : true)

clang_arguments = c_optimization_options
clang_arguments += ['@EXTRA_ARGS@', '-I', meson.source_root() / 'src',
  '-I', '@CURRENT_SOURCE_DIR@', '@INPUT@', '-c', '-o', '@OUTPUT@',
  '-Xclang', '-load', '-Xclang', rootsim_llvm_lib.full_path()]

clang_with_plugin = generator(clang_compiler, output : '@BASENAME@.o',
  arguments : clang_arguments, depends : rootsim_llvm_lib)
