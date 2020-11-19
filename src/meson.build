compiler_c_args = [
  '-DROOTSIM_LIB_DIR="' + get_option('prefix') / get_option('libdir') / '"',
  '-DROOTSIM_INC_DIR="' + get_option('prefix') / get_option('includedir') / '"'
]

# Meson build options and common macros
if get_option('buildtype') == 'release'
  log_level_def = '-DLOG_LEVEL=LOG_INFO'
else
  log_level_def = '-DLOG_LEVEL=LOG_DEBUG'
  compiler_c_args += '-g'
endif

serial_c_args = [log_level_def, '-DROOTSIM_SERIAL']
parallel_c_args = [log_level_def]

if get_option('b_coverage')
  parallel_c_args += '-DROOTSIM_COVERAGE=1'
endif

llvm_plugin_cpp_args = [log_level_def]

# Include directory used in tests and llvm plugin compilation
src_inc_dir = include_directories('.')

# LLVM library dependency
llvm_dep = dependency('llvm', version : '>= 9.0',
  required : get_option('incremental_state_saving'))
if llvm_dep.found()
  parallel_c_args += '-DROOTSIM_INCREMENTAL=1'
  compiler_c_args += '-DROOTSIM_INCREMENTAL=1'
  rootsim_header = files('ROOT-Sim.h')
  subdir('instr')
endif

# MPI library dependency
mpi_dep = dependency('mpi', language : 'c', required : get_option('mpi'))
if mpi_dep.found()
  parallel_c_args += '-DROOTSIM_MPI=1'
  compiler_c_args += '-DROOTSIM_MPI=1'
endif

# Math library dependency
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

# Threads library dependency
threads_dep = dependency('threads')

# Model helper library sources
lib_srcs = [
  'lib' / 'random' / 'random.c',
  'lib' / 'state' / 'state.c',
  'lib' / 'topology' / 'topology.c',
  'lib' / 'lib.c'
]

# ROOT-Sim serial library sources
serial_srcs = [
  'core' / 'arg_parse.c',
  'core' / 'core.c',
  'core' / 'init.c',
  'log' / 'log.c',
  'log' / 'stats.c',
  'mm' / 'mm.c',
  'mm' / 'msg_allocator.c',
  'serial' / 'serial.c'
]

serial_srcs += lib_srcs

rootsim_serial_lib = static_library('rootsim-serial', serial_srcs,
  c_args : serial_c_args, dependencies: m_dep, install : true)

# ROOT-Sim parallel library sources
parallel_srcs = [
  'arch' / 'arch.c',
  'arch' / 'threads.c',
  'core' / 'arg_parse.c',
  'core' / 'core.c',
  'core' / 'init.c',
  'core' / 'sync.c',
  'datatypes' / 'msg_queue.c',
  'datatypes' / 'remote_msg_map.c',
  'distributed' / 'mpi.c',
  'gvt' / 'fossil.c',
  'gvt' / 'gvt.c',
  'gvt' / 'termination.c',
  'log' / 'log.c',
  'log' / 'stats.c',
  'lp' / 'lp.c',
  'lp' / 'process.c',
  'mm' / 'mm.c',
  'mm' / 'msg_allocator.c',
  'parallel' / 'parallel.c'
]

# Choose from the available memory allocators
if get_option('memory_allocator') == 'buddy'
  parallel_c_args += '-DROOTSIM_BUDDY_ALLOCATOR=1'
  parallel_srcs += 'mm' / 'buddy' / 'buddy.c'
else
  parallel_srcs += 'mm' / 'minm' / 'minm.c'
endif

parallel_srcs += lib_srcs

rootsim_parallel_lib = static_library('rootsim-parallel', parallel_srcs,
  c_args : parallel_c_args, dependencies : [mpi_dep, m_dep], install : true)

# ROOT-Sim compiler
executable('rootsim-cc', sources : 'compiler.c', c_args : compiler_c_args, install : true)

# Install headers
install_headers('ROOT-Sim.h')
