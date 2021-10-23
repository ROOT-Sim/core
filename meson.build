# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

project('ROOT-Sim', ['c', 'cpp'], version : '3.0.0-alpha.3',
  meson_version : '>=0.55', default_options : ['c_std=c11', 'cpp_std=c++14',
  'buildtype=release', 'warning_level=3', 'unity=on', 'unity_size=10000',
  'b_lto=true', 'b_pie=false', 'b_staticpic=false'])

# This is needed since our compiler uses a LLVM plugin
assert(meson.get_compiler('c').get_id() == 'clang' or not get_option('b_lto'),
  'LTO builds are only supported with the clang compiler;\neither pass ' +
  '\'-Db_lto=false\' to the meson command or prepend it with \'env CC=clang ' +
  'CXX=clang++\'')

# Workaround to have reproducible unity builds
if get_option('unity') == 'on'
  if meson.get_compiler('c').has_argument('-ffile-prefix-map=.=.')
    prefix_arg = '-ffile-prefix-map=' + meson.build_root() / '..=..'
    add_global_arguments(prefix_arg, language : 'c')
  endif
endif

# Used in some defines, the llvm plugin and the tests
if get_option('buildtype') == 'release'
  c_optimization_options = ['-Ofast', '-march=native', '-mtune=native']
  add_global_arguments(c_optimization_options, language : 'c')
  add_global_link_arguments(c_optimization_options, language : 'c')
else
  c_optimization_options = ['-O' + get_option('optimization')]
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
clang_compiler = find_program(llvm_dep.get_variable(configtool : 'bindir') /
  'clang')

if get_option('debug')
  c_optimization_options += '-g'
endif
if get_option('b_lto')
  c_optimization_options += '-flto'
endif
if get_option('b_sanitize') == 'address'
  c_optimization_options += '-fsanitize=address'
endif
if llvm_dep.version().version_compare('>= 13')
  c_optimization_options += '-flegacy-pass-manager'
endif

# Logging level define
if get_option('debug')
  log_level_def = '-DLOG_LEVEL=LOG_TRACE'
else
  log_level_def = '-DLOG_LEVEL=LOG_INFO'
endif

# Custom threads library dependency
rs_thr_dep = declare_dependency(link_with :
  static_library('rootsim-threads', 'src' / 'arch' / 'thread.c',
    dependencies: dependency('threads'), include_directories : src_inc_dir))

# Used to build the LLVM plugin
llvm_cfg_gen = find_program('scripts' / 'llvm_cfg_gen.py')

subdir('src')
subdir('test')
subdir('docs')
