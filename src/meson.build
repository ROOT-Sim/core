# math library dependency
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

# mpi library dependency
mpi_dep = dependency('mpi', language: 'c')

# threads library dependency
threads_dep = dependency('threads')

# model helper library sources
lib_srcs = [
  'lib/random/random.c',
  'lib/state/state.c',
  'lib/lib.c'
]

# neurome serial library sources
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
  c_args : ['-DNEUROME_SERIAL', '-DLOG_LEVEL=LOG_DEBUG'], dependencies: m_dep)

# neurome parallel library sources
parallel_srcs = [
  'arch/arch.c',
  'core/core.c',
  'core/init.c',
  'core/sync.c',
  'datatypes/msg_queue.c',
  'distributed/mpi.c',
  'gvt/gvt.c',
  'gvt/termination.c',
  'log/log.c',
  'lp/lp.c',
  'lp/msg.c',
  'lp/process.c',
  'mm/model_allocator.c',
  'mm/msg_allocator.c',
  'parallel/parallel.c'
]

parallel_srcs += lib_srcs

neurome_parallel_lib = static_library('neurome-parallel', parallel_srcs, c_args : ['-DLOG_LEVEL=LOG_DEBUG'], dependencies : m_dep)

# neurome compiler
executable('neurome-cc', sources : 'compiler.c')
