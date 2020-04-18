# Meson build options and common macros
if get_option('buildtype') == 'release'
  log_level_def = '-DLOG_LEVEL=LOG_INFO'
else 
  log_level_def = '-DLOG_LEVEL=LOG_DEBUG'
endif

serial_c_args = [log_level_def, '-DNEUROME_SERIAL']
parallel_c_args = [log_level_def]

if get_option('b_coverage')
  parallel_c_args += '-DHAVE_COVERAGE=1'
endif

compiler_c_args = [
  '-DNEUROME_LIB_DIR="' + get_option('prefix') / get_option('libdir') / '"', 
  '-DNEUROME_INC_DIR="' + get_option('prefix') / get_option('includedir') / '"'
]
  
# Math library dependency
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

# MPI library dependency
mpi_dep = dependency('mpi', language: 'c', required : false)

# Threads library dependency
threads_dep = dependency('threads')

# Model helper library sources
lib_srcs = [
  'lib/random/random.c',
  'lib/state/state.c',
  'lib/topology/topology.c',
  'lib/lib.c'
]

# NeuRome serial library sources
serial_srcs = [
  'core/core.c',
  'core/init.c',
  'log/log.c',
  'log/stats.c',
  'mm/msg_allocator.c',
  'serial/serial.c'
]

serial_srcs += lib_srcs

neurome_serial_lib = static_library('neurome-serial', serial_srcs, 
  c_args : serial_c_args, dependencies: m_dep, install : true)

# NeuRome parallel library sources
parallel_srcs = [
  'arch/arch.c',
  'core/core.c',
  'core/init.c',
  'core/sync.c',
  'datatypes/msg_queue.c',
  'distributed/mpi.c',
  'gvt/fossil.c',
  'gvt/gvt.c',
  'gvt/termination.c',
  'log/log.c',
  'log/stats.c',
  'lp/lp.c',
  'lp/msg.c',
  'lp/process.c',
  'mm/model_allocator.c',
  'mm/msg_allocator.c',
  'parallel/parallel.c'
]

parallel_srcs += lib_srcs

neurome_parallel_lib = static_library('neurome-parallel', parallel_srcs, 
  c_args : parallel_c_args, dependencies : m_dep, install : true)

# NeuRome compiler
executable('neurome-cc', sources : 'compiler.c', c_args : compiler_c_args, install : true)

# Install headers
install_headers('NeuRome.h', 'ROOT-Sim.h')

# Include directory used in tests
src_inc_dir = include_directories('.')
