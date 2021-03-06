# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

# Build the LLVM instrumentation plugin
# TODO re-enable incremental state saving
subdir('instr')
# Build the model libraries
# TODO integrate the plugin system
subdir('lib')

rootsim_version_def = '-DROOTSIM_VERSION="' + meson.project_version() + '"'
rootsim_c_args = [rootsim_version_def, log_level_def]

# ROOT-Sim library sources
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

# ROOT-Sim compiler defines
compiler_c_args = [
  '-DROOTSIM_LIB_DIR="' + get_option('prefix') / get_option('libdir') / '"',
  '-DROOTSIM_INC_DIR="' + get_option('prefix') / get_option('includedir') / '"',
  '-DROOTSIM_OPTIMIZATION_OPTIONS="' + ' '.join(c_optimization_options) + '"'
]

if mpi_dep.found()
  compiler_c_args += '-DROOTSIM_CC="env OMPI_CC=' + clang_compiler.full_path() + ' MPICH_CC=' + clang_compiler.full_path() + ' mpicc"'
else
  compiler_c_args += '-DROOTSIM_CC="' + clang_compiler.full_path() + '"'
endif

# ROOT-Sim compiler
rootsim_cc = executable('rootsim-cc', sources : 'compiler' / 'compiler.c', 
  c_args : compiler_c_args, install : true)

# Install headers
install_headers('ROOT-Sim.h')
