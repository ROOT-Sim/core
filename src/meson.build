# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

######################
#  ROOT-Sim Kernel   #
######################

rootsim_version_def = '-DROOTSIM_VERSION="' + meson.project_version() + '"'
rootsim_c_args = [rootsim_version_def, log_level_def]

# ROOT-Sim core sources
rootsim_srcs = [
  'arch' / 'io.c',
  'arch' / 'mem.c',
  'arch' / 'timer.c',
  'core' / 'arg_parse.c',
  'core' / 'core.c',
  'core' / 'init.c',
  'core' / 'sync.c',
  'datatypes' / 'msg_queue.c',
  'gvt' / 'fossil.c',
  'gvt' / 'gvt.c',
  'gvt' / 'termination.c',
  'log' / 'log.c',
  'log' / 'stats.c',
  'lp' / 'lp.c',
  'lp' / 'process.c',
  'mm' / 'buddy' / 'buddy.c',
  'mm' / 'mm.c',
  'mm' / 'msg_allocator.c',
  'parallel' / 'parallel.c',
  'serial' / 'serial.c',
  'main.c'
]

if mpi_dep.found()
  rootsim_c_args += '-DROOTSIM_MPI=1'
  rootsim_srcs += 'distributed' / 'mpi.c'
else
  rootsim_srcs += 'distributed' / 'no_mpi.c'
endif

rootsim_lib = static_library('rootsim', rootsim_srcs, c_args : rootsim_c_args,
  dependencies : [mpi_dep, m_dep, rs_thr_dep], install : true)

######################
# ROOT-Sim Compiler  #
######################

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
rootsim_llvm_lib = shared_library('rootsim-cc_llvm', ['compiler' / 'rootsim-cc_llvm.cpp', instr_cfg],
  dependencies : rootsim_llvm_lib_dep, name_prefix : 'lib', name_suffix : 'so',
  cpp_args : rootsim_llvm_lib_cpp_args, include_directories : src_inc_dir,
  install : true)

clang_arguments = c_optimization_options
clang_arguments += ['@EXTRA_ARGS@', '-I', meson.source_root() / 'src',
  '-I', '@CURRENT_SOURCE_DIR@', '@INPUT@', '-c', '-o', '@OUTPUT@',
  '-Xclang', '-load', '-Xclang', rootsim_llvm_lib.full_path()]

clang_with_plugin = generator(clang_compiler, output : '@BASENAME@.o',
  arguments : clang_arguments, depends : rootsim_llvm_lib)


# ROOT-Sim compiler defines
compiler_c_args = [
  '-DROOTSIM_LIB_DIR="' + get_option('prefix') / get_option('libdir') / '"',
  '-DROOTSIM_INC_DIR="' + get_option('prefix') / get_option('includedir') / '"',
  '-DROOTSIM_OPTIMIZATION_OPTIONS="' + ' '.join(c_optimization_options) + '"'
]

clang_path = clang_compiler.full_path()
if mpi_dep.found()
  compiler_c_args += '-DROOTSIM_CC="env OMPI_CC=' + clang_path + ' MPICH_CC=' + clang_path + ' mpicc"'
else
  compiler_c_args += '-DROOTSIM_CC="' + clang_path + '"'
endif

# Build the compiler
rootsim_cc = executable('rootsim-cc', sources : 'compiler' / 'rootsim-cc.c',
  c_args : compiler_c_args, install : true)

######################
#  Model Libraries   #
######################
# Model helper library sources
lib_srcs = files(
  'lib' / 'random' / 'random.c',
  'lib' / 'state' / 'state.c',
  'lib' / 'topology' / 'topology.c',
  'lib' / 'config' / 'jsmn.c',
  'lib' / 'config' / 'reflect.c',
  'lib' / 'lib.c'
)

lib_objs = clang_with_plugin.process(lib_srcs)
rootsim_mods_lib = static_library('rootsim-mods', lib_objs, install : true)

######################
#  ROOT-Sim Header   #
######################
install_headers('ROOT-Sim.h')
