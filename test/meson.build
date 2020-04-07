src_dir = '../src/'
# main include directory
incdir = include_directories(src_dir)
# Declare test dependencies
serial_lib_dep = declare_dependency(link_with: neurome_serial_library)
thread_lib_dep = dependency('threads')

# Tests names and sources
tests_specs = {
  'msg_allocator' : {
    'sources' : [
      'test.c',
      'mm/msg_allocator_test.c', 
      src_dir + 'mm/msg_allocator.c'
    ],
    'dependencies' : [ thread_lib_dep ]
  },
  'model_allocator' : {
    'sources' : [
      'test.c',
      'mm/model_allocator_test.c',
      src_dir + 'mm/model_allocator.c',
    ],
    'dependencies' : [ thread_lib_dep ]
  },
  'msg_queue' : {
    'sources' : [
      'test.c',
      'datatypes/msg_queue_test.c',
      src_dir + 'core/core.c',
      src_dir + 'datatypes/msg_queue.c',
    ],
    'dependencies' : [ thread_lib_dep ]
  },
  'integration_serial' : {
    'sources' : [
      'test.c',
      'integration/application.c', 
      'integration/functions.c',
      'integration/output.c'
    ],
    'dependencies' : [ thread_lib_dep, serial_lib_dep ]
  }
}

# Actual build of the tests
i = 1
foreach test_name, test_specs : tests_specs
  sources_arr = test_specs.get('sources')
  dependencies_arr = test_specs.get('dependencies')
  e = executable(
    'test_' + test_name, 
    sources : sources_arr, 
    dependencies : test_specs.get('dependencies'), 
    link_args : '-Wl,--wrap=main', 
    include_directories : incdir
  )
  test('test_' + i.to_string() + '_' + test_name, e)
  i = i + 1
endforeach
