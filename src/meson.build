# Main artifacts
serial_srcs = [
  'serial/serial.c',
  'mm/msg_allocator.c',
  'lp/lp.c',
  'log/log.c',
  'core/init.c'
]
serial_lib = static_library('neurome-serial', serial_srcs, c_args : ['-DNEUROME_SERIAL', '-DLOG_LEVEL=LOG_DEBUG'])

# Declare test dependencies
serial_lib_dep = declare_dependency(link_with: serial_lib)
thread_lib_dep = dependency('threads')

# Tests names and sources
tests_specs = {
  'msg_allocator' : {
    'sources' : [
      'test/test.c',
      'test/mm/msg_allocator_test.c', 
      'mm/msg_allocator.c'
    ],
    'dependencies' : [ thread_lib_dep ]
  },
  'model_allocator' : {
    'sources' : [
      'test/test.c',
      'test/mm/model_allocator_test.c', 
      'lp/lp.c',
      'mm/model_allocator.c'
    ],
    'dependencies' : [ thread_lib_dep ]
  },
  'integration_serial' : {
    'sources' : [
      'test/test.c',
      'test/integration_serial/application.c', 
      'test/integration_serial/functions.c',
      'test/integration_serial/output.c'
    ],
    'dependencies' : [ thread_lib_dep, serial_lib_dep ]
  }
}

# Actual build of the tests
i = 1
foreach test_name, test_specs : tests_specs
  sources_arr = test_specs.get('sources')
  dependencies_arr = test_specs.get('dependencies')
  e = executable('test_' + test_name, sources : sources_arr, dependencies : test_specs.get('dependencies'), link_args : '-Wl,--wrap=main')
  test('test_' + i.to_string() + '_' + test_name, e)
  i = i + 1
endforeach

executable('neurome-cc', sources : 'compiler.c')


