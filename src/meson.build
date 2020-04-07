lib_srcs = [
  'lib/state/state.c',
  'lib/lib.c'
]
# Main artifacts
serial_srcs = [
  'core/core.c',
  'core/init.c',
  'log/log.c',
  'mm/msg_allocator.c',
  'serial/serial.c'
]

serial_srcs += lib_srcs

neurome_serial_library = static_library('neurome-serial', serial_srcs, c_args : ['-DNEUROME_SERIAL', '-DLOG_LEVEL=LOG_DEBUG'])

parallel_srcs = [
  'arch/arch.c',
  'core/core.c',
  'core/init.c',
  'core/sync.c',
  'datatypes/msg_queue.c',
  'distributed/mpi.c',
  'gvt/gvt.c',
  'log/log.c',
  'lp/lp.c',
  'lp/msg.c',
  'lp/process.c',
  'mm/model_allocator.c',
  'mm/msg_allocator.c',
  'parallel/parallel.c'
]

parallel_srcs += lib_srcs

mpi_dep = dependency('mpi', language: 'c')

neurome_parallel_library = static_library('neurome-parallel', parallel_srcs, c_args : ['-DLOG_LEVEL=LOG_DEBUG'], dependencies: mpi_dep)

executable('neurome-cc', sources : 'compiler.c')
