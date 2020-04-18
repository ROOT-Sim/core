src_dir = meson.source_root() / 'src'
models_dir = meson.source_root() / 'models'
# Test dependencies
serial_dep = declare_dependency(link_whole: neurome_serial_lib)
parallel_dep = declare_dependency(link_whole: neurome_parallel_lib, 
  link_args: '-Wl,--wrap=malloc,--wrap=realloc,--wrap=free,--wrap=calloc')

fail_tests = [
  'init',
  'body',
  'fini',
  'main',
  'cmp_len',
  'cmp_str'
]

# Fail tests builds
i = 1
foreach test_name : fail_tests
  e = executable(
    'test_f' + i.to_string() + '_' + test_name, 
    sources : ['test.c', 'test' / 'fail_' + test_name + '.c'], 
    dependencies : threads_dep, 
    c_args : ['-DNEUROME_TEST'],
    link_args : '-Wl,--wrap=main', 
  )
  test('test_f' + i.to_string() + '_' + test_name, e, should_fail: true)
  i = i + 1
endforeach


# Unit tests names and sources
unit_tests = {
  'sync' : 'core' / 'sync',
  'msg_allocator' : 'mm' / 'msg_allocator',
  'gvt' : 'gvt' / 'gvt',
  'lp' : 'lp' / 'lp',
  'model_allocator' : 'mm' / 'model_allocator',
  'msg_queue' : 'datatypes' / 'msg_queue'
}

# Unit tests builds
i = 1
foreach test_name, test_path : unit_tests
  e = executable(
    'test_u' + i.to_string() + '_' + test_name, 
    sources : ['test.c', test_path + '_test.c', src_dir / test_path + '.c'], 
    dependencies : threads_dep, 
    c_args : ['-DNEUROME_TEST'],
    link_args : '-Wl,--wrap=main', 
    include_directories : src_inc_dir
  )
  test('test_u' + i.to_string() + '_' + test_name, e)
  i = i + 1
endforeach

# Common integration tests srcs
integration_srcs = [
  'test.c',
  'integration' / 'model' / 'application.c', 
  'integration' / 'model' / 'functions.c',
  'integration' / 'model' / 'output.c'
]

# Integration tests specs
integration_tests = {
  'serial': {
    'src' : 'integration' / 'integration_serial.c',
    'dep' : serial_dep
  },
  'parallel_single':  {
    'src' : 'integration' / 'integration_parallel_single.c',
    'dep' : parallel_dep
  },
  'parallel_multi': {
    'src' : 'integration' / 'integration_parallel_multi.c',
    'dep' : parallel_dep
  }
}

# Integration tests builds
i = 1
foreach test_name, test_spec : integration_tests
  e = executable(
    'test_i' + i.to_string() + '_' + test_name, 
    sources : integration_srcs + [test_spec.get('src')], 
    dependencies : [threads_dep, test_spec.get('dep')], 
    c_args : ['-DNEUROME_TEST'],
    link_args : '-Wl,--wrap=main', 
    include_directories : src_inc_dir
  )
  test('test_i' + i.to_string() + '_' + test_name, e, is_parallel: false)
  i = i + 1
endforeach

models_tests = {
  'pcs' : {
    'srcs' : ['application.c', 'functions_app.c'],
    'lps' : 16
  },
  'phold' : {
    'srcs' : ['application.c'],
    'lps' : 256
  }
}

i = 1
foreach model_name, test_spec : models_tests
  model_srcs = []
  foreach src_name : test_spec.get('srcs')
    model_srcs += [models_dir / model_name / src_name]
  endforeach
  
  e = executable(
    'test_m' + i.to_string() + '_' + model_name, 
    sources : ['test.c', 'models' / 'model_test.c'] + model_srcs,
    dependencies : [threads_dep, parallel_dep], 
    c_args : [
      '-DNEUROME_TEST', 
      '-DLPS_COUNT="' + test_spec.get('lps').to_string() + '"',
      '-DMODEL_NAME="' + model_name + '"'
    ],
    link_args : '-Wl,--wrap=main', 
    include_directories : src_inc_dir
  )
  test('test_m' + i.to_string() + '_' + model_name, e, is_parallel: false)
  i = i + 1
endforeach
