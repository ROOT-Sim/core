src_dir = '../src/'
# main include directory
incdir = include_directories(src_dir)
# Declare test dependencies
serial_dep = declare_dependency(link_whole: neurome_serial_lib)
parallel_dep = declare_dependency(link_whole: neurome_parallel_lib, 
  link_args: '-Wl,--wrap=malloc,--wrap=realloc,--wrap=free,--wrap=calloc')

# Tests names and sources
tests_specs = {
  'sync' : {
    'sources' : [
      'test.c',
      'core/sync_test.c', 
      src_dir + 'core/core.c',
      src_dir + 'core/sync.c'
    ],
    'dependencies' : [ threads_dep ]
  },
  'msg_allocator' : {
    'sources' : [
      'test.c',
      'mm/msg_allocator_test.c', 
      src_dir + 'mm/msg_allocator.c'
    ],
    'dependencies' : [ threads_dep ]
  },
  'gvt' : {
    'sources' : [
      'test.c',
      'gvt/gvt_test.c', 
      src_dir + 'core/core.c',
      src_dir + 'gvt/gvt.c'
    ],
    'dependencies' : [ threads_dep ]
  },
  'lp' : {
    'sources' : [
      'test.c',
      'lp/lp_test.c', 
      src_dir + 'core/core.c',
      src_dir + 'lp/lp.c'
    ],
    'dependencies' : [ threads_dep ]
  },
  'model_allocator' : {
    'sources' : [
      'test.c',
      'mm/model_allocator_test.c',
      src_dir + 'mm/model_allocator.c',
    ],
    'dependencies' : [ threads_dep ]
  },
  'msg_queue' : {
    'sources' : [
      'test.c',
      'datatypes/msg_queue_test.c',
      src_dir + 'core/core.c',
      src_dir + 'datatypes/msg_queue.c',
    ],
    'dependencies' : [ threads_dep ]
  },
  'integration_serial' : {
    'sources' : [
      'test.c',
      'integration/integration_serial.c', 
      'integration/model/application.c', 
      'integration/model/functions.c',
      'integration/model/output.c'
    ],
    'dependencies' : [ threads_dep, serial_dep ]
  },
  'integration_parallel_single' : {
    'sources' : [
      'test.c',
      'integration/integration_parallel_single.c',
      'integration/model/application.c', 
      'integration/model/functions.c',
      'integration/model/output.c'
    ],
    'dependencies' : [ threads_dep, parallel_dep ]
  },
  'integration_parallel_multi' : {
    'sources' : [
      'test.c',
      'integration/integration_parallel_multi.c',
      'integration/model/application.c', 
      'integration/model/functions.c',
      'integration/model/output.c'
    ],
    'dependencies' : [ threads_dep, parallel_dep ]
  }
}

# Actual build of the tests
i = 1
foreach test_name, test_specs : tests_specs
  e = executable(
    'test_' + test_name, 
    sources : test_specs.get('sources'), 
    dependencies : test_specs.get('dependencies'), 
    c_args : ['-DNEUROME_TEST'],
    link_args : '-Wl,--wrap=main', 
    include_directories : incdir
  )
  test('test_' + i.to_string() + '_' + test_name, e)
  i = i + 1
endforeach
