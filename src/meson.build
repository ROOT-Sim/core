# Main artifacts
serial_srcs = [
  'serial/serial.c',
  'lib/lib.c',
  'mm/msg_allocator.c',
  'log/log.c',
  'core/init.c'
]
neurome_serial_library = static_library('neurome-serial', serial_srcs, c_args : ['-DNEUROME_SERIAL', '-DLOG_LEVEL=LOG_DEBUG'])

executable('neurome-cc', sources : 'compiler.c')
