# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
project('ROOT-Sim', ['c', 'cpp'], version : '3.0.0',
  default_options : ['c_std=c11', 'cpp_std=c++14', 'buildtype=release',
  'warning_level=3', 'unity=on', 'unity_size=10000', 'b_lto=true'])

# This is needed since our compiler uses a LLVM plugin
assert(meson.get_compiler('c').get_id() == 'clang' or not get_option('b_lto'),
  'LTO builds are only supported with the clang compiler;\neither pass ' +
  '\'-Db_lto=false\' to the meson command or prepend it with \'env CC=clang ' +
  'CXX=clang++\'')

# This stuff would otherwise break our fragile linking process
add_global_link_arguments(['-no-pie'], language : 'c')
if meson.get_compiler('c').get_id() == 'gcc'
  add_global_arguments(['-no-pie', '-fno-pic'], language : 'c')
endif

# Workaround to have reproducible unity builds
if get_option('unity') == 'on'
  add_global_arguments(['-ffile-prefix-map=' + meson.build_root() / '..=..'], 
    language : 'c')
endif

# Include directory used explicitly in tests and LLVM plugin compilation
src_inc_dir = include_directories('src')

# C math library dependency
m_dep = meson.get_compiler('c').find_library('m', required : false)

# LLVM library dependency
llvm_dep = dependency('llvm', version : '>= 9.0', required : true)

# MPI library dependency
mpi_dep = dependency('mpi', language : 'c', required : get_option('mpi'))

# Clang compiler
clang_compiler = find_program('clang')

# Custom threads library dependency
rs_thr_dep = declare_dependency(link_with :
  static_library('rootsim-threads', 'src' / 'arch' / 'thread.c',
    dependencies: dependency('threads'), include_directories : src_inc_dir))

# Used in some defines, the llvm plugin and the tests    
c_optimization_options = ['-O' + get_option('optimization')]
if get_option('debug')
  c_optimization_options += '-g'
endif
if get_option('b_lto')
  c_optimization_options += '-flto'
endif

# Logging level define
if get_option('debug')
  log_level_def = '-DLOG_LEVEL=LOG_TRACE'
else
  log_level_def = '-DLOG_LEVEL=LOG_INFO'
endif

subdir('src')
subdir('test')
subdir('docs')
