src_dir = meson.source_root() / 'src'
models_dir = meson.source_root() / 'models'
# Test dependencies
rootsim_dep = declare_dependency(link_with : [rootsim_lib, rootsim_mods_lib])

test_model_srcs = [
  'integration' / 'model' / 'application.c',
  'integration' / 'model' / 'functions.c',
  'integration' / 'model' / 'output.c'
]

test_model_objs = clang_with_plugin.process(test_model_srcs)
test_model_dep = declare_dependency(link_with : static_library('test-model', 
  test_model_objs))

test_specs = {
  'fail_init': {'src' : ['test' / 'fail_init.c'], 'fail' : true},
  'fail_body': {'src' : ['test' / 'fail_body.c'], 'fail' : true},
  'fail_fini': {'src' : ['test' / 'fail_fini.c'], 'fail' : true},
  'fail_main': {'src' : ['test' / 'fail_main.c'], 'fail' : true, 'has_main' : true},
  'fail_cmp_short': {'src' : ['test' / 'fail_cmp_short.c'], 'fail' : true, 'has_main' : true},
  'fail_cmp_long': {'src' : ['test' / 'fail_cmp_long.c'], 'fail' : true, 'has_main' : true},
  'fail_cmp_str': {'src' : ['test' / 'fail_cmp_str.c'], 'fail' : true, 'has_main' : true},
  
  'unit_init' : {'src' : ['core' / 'init_test.c', src_dir / 'core' / 'init.c', src_dir / 'core' / 'arg_parse.c'], 'has_main' : true},
  'unit_sync' : {'src' : ['core' / 'sync_test.c', src_dir / 'core' / 'sync.c']},
  'unit_msg_queue' : {'src' : ['datatypes' / 'msg_queue_test.c', src_dir / 'datatypes' / 'msg_queue.c']},
  'unit_remote_msg_map' : {'src' : ['datatypes' / 'remote_msg_map_test.c', src_dir / 'datatypes' / 'remote_msg_map.c']},
# 'unit_gvt' : {'src' : ['gvt' / 'gvt_test.c', src_dir / 'gvt' / 'gvt.c']},
  'unit_msg_allocator' : {'src' : ['mm' / 'msg_allocator_test.c', src_dir / 'mm' / 'msg_allocator.c']},
  'unit_lp' : {'src' : ['lp' / 'lp_test.c', src_dir / 'lp' / 'lp.c']},
  'unit_buddy_allocator' : {'src' : ['mm' / 'buddy' / 'buddy_test.c', src_dir / 'mm' / 'buddy' / 'buddy.c']},
  'unit_compiler' : {'src' : ['compiler' / 'compiler_test.c', src_dir / 'compiler' / 'compiler.c'], 'has_main' : true},

  'integration_serial': {
    'src' : ['integration' / 'integration_serial.c'],
    'has_main' : true,
    'dep' : [rootsim_dep, test_model_dep]
  },
  'integration_single':  {
    'src' : ['integration' / 'integration_parallel_single.c'],
    'has_main' : true,
    'dep' : [rootsim_dep, test_model_dep]
  },
  'integration_multi': {
    'src' : ['integration' / 'integration_parallel_multi.c'],
    'has_main' : true,
    'dep' : [rootsim_dep, test_model_dep]
  },
}
kappa = {  
  'model_pcs': {
    'src' : [
      'models' / 'model_test.c', 
      models_dir / 'pcs' / 'application.c', 
      models_dir / 'pcs' / 'functions_app.c'
    ],
    'has_main' : true,
    'dep' : [rootsim_dep]
  },
  'model_phold': {
    'src' : [
      'models' / 'model_test.c', 
      models_dir / 'phold' / 'application.c', 
    ],
    'has_main' : true,
    'dep' : [rootsim_dep]
  }
}

# Tests builds
i = 1
foreach test_name, test_spec : test_specs
  test_srcs = test_spec.get('src')
  test_srcs += 'test.c'
  if not test_spec.get('has_main', false)
    test_srcs += 'test_main.c'
  endif
  
  full_test_name = 'test_' + i.to_string() + '_' + test_name
  
  test_executable = executable(
    full_test_name,
    sources : test_srcs,
    dependencies : [rs_thr_dep] + test_spec.get('dep', []),
    c_args : ['-DROOTSIM_TEST', '-DROOTSIM_TEST_NAME="' + test_name + '"'],
    include_directories : src_inc_dir,
    link_args : '-no-pie'
  )
  
  test(
    full_test_name, 
    test_executable, 
    should_fail: test_spec.get('fail', false), 
    is_parallel: false
  )
  
  i = i + 1
endforeach
