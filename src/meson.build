# Meson build options and common macros
if get_option('buildtype') == 'release'
  log_level_def = '-DLOG_LEVEL=LOG_INFO'
else
  log_level_def = '-DLOG_LEVEL=LOG_DEBUG'
endif

rootsim_c_args = [log_level_def]

# TODO maybe not needed anymore
if get_option('b_coverage')
  rootsim_c_args += '-DROOTSIM_COVERAGE=1'
endif

#  parallel_c_args += '-DROOTSIM_INCREMENTAL=1'
#  compiler_c_args += '-DROOTSIM_INCREMENTAL=1'
subdir('instr')
subdir('lib')

if mpi_dep.found()
  rootsim_c_args += '-DROOTSIM_MPI=1'
endif

# ROOT-Sim library sources
rootsim_srcs = [
  'core' / 'arg_parse.c',
  'core' / 'core.c',
  'core' / 'init.c',
  'core' / 'sync.c',
  'core' / 'timer.c',
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
  'mm' / 'buddy' / 'buddy.c',
  'mm' / 'mm.c',
  'mm' / 'msg_allocator.c',
  'parallel' / 'parallel.c',
  'serial' / 'serial.c',
  'main.c'
]

rootsim_lib = static_library('rootsim', rootsim_srcs, c_args : rootsim_c_args,
  dependencies : [mpi_dep, m_dep, rs_thr_dep], install : true)

# ROOT-Sim compiler defines
compiler_c_args = [
  '-DROOTSIM_LIB_DIR="' + get_option('prefix') / get_option('libdir') / '"',
  '-DROOTSIM_INC_DIR="' + get_option('prefix') / get_option('includedir') / '"',
  '-DROOTSIM_CC="' + clang_compiler.full_path() + '"'
]

# ROOT-Sim compiler
rootsim_cc = executable('rootsim-cc', sources : 'compiler' / 'compiler.c', 
  c_args : compiler_c_args, install : true)

# Install headers
install_headers('ROOT-Sim.h')
