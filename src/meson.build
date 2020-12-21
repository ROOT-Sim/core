# Build the LLVM instrumentation plugin
# TODO re-enable incremental state saving
subdir('instr')
# Build the model libraries
# TODO integrate the plugin system
subdir('lib')

rootsim_c_args = [log_level_def]

if mpi_dep.found()
  rootsim_c_args += '-DROOTSIM_MPI=1'
endif

# ROOT-Sim library sources
rootsim_srcs = [
  'arch' / 'io.c',
  'arch' / 'timer.c',
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
  '-DROOTSIM_CC="' + clang_compiler.full_path() + '"',
  '-DROOTSIM_OPTIMIZATION_OPTIONS="' + ' '.join(c_optimization_options) + '"'
]

# ROOT-Sim compiler
rootsim_cc = executable('rootsim-cc', sources : 'compiler' / 'compiler.c', 
  c_args : compiler_c_args, install : true)

# Install headers
install_headers('ROOT-Sim.h')
