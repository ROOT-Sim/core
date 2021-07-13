# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

fs = import('fs')

src_dir = meson.source_root() / 'src'
models_dir = meson.source_root() / 'models'
# Test dependencies
rootsim_dep = declare_dependency(link_with : [rootsim_lib, rootsim_mods_lib])

model_runner = find_program(meson.source_root() / 'scripts' / 'model_runner.py')

test_model_srcs = [
  'integration' / 'model' / 'application.c',
  'integration' / 'model' / 'functions.c',
  'integration' / 'model' / 'output_256.c'
]

test_specs = {
  'fail_init': {'src' : ['self' / 'fail_init.c'], 'fail' : true},
  'fail_body': {'src' : ['self' / 'fail_body.c'], 'fail' : true},
  'fail_fini': {'src' : ['self' / 'fail_fini.c'], 'fail' : true},
  'fail_main': {'src' : ['self' / 'fail_main.c'], 'fail' : true, 'has_main' : true},

  'unit_init' : {'src' : ['core' / 'init_test.c', src_dir / 'core' / 'init.c', src_dir / 'core' / 'arg_parse.c'], 'has_main' : true},
  'unit_sync' : {'src' : ['core' / 'sync_test.c', src_dir / 'core' / 'sync.c']},
  'unit_msg_queue' : {'src' : ['datatypes' / 'msg_queue_test.c', src_dir / 'datatypes' / 'msg_queue.c']},
# 'unit_gvt' : {'src' : ['gvt' / 'gvt_test.c', src_dir / 'gvt' / 'gvt.c']},
  'unit_msg_allocator' : {'src' : ['mm' / 'msg_allocator_test.c', src_dir / 'mm' / 'msg_allocator.c']},
  'unit_lp' : {'src' : ['lp' / 'lp_test.c', src_dir / 'lp' / 'lp.c']},
  'unit_buddy_allocator' : {'src' : ['mm' / 'buddy' / 'buddy_test.c', src_dir / 'mm' / 'buddy' / 'buddy.c']},
  'unit_compiler' : {'src' : ['compiler' / 'compiler_test.c', src_dir / 'compiler' / 'rootsim-cc.c'], 'has_main' : true, 'c_args' : compiler_c_args},
  'unit_reflect' : {'src' : ['lib' / 'reflect.c', src_dir / 'lib' / 'config' / 'jsmn.c', src_dir / 'lib' / 'config' / 'reflect.c', src_dir / 'datatypes' / 'vector.c']},

  'integration_serial': {
    'src' : ['integration' / 'integration_serial.c'] + test_model_srcs,
    'rslib' : true
  },
  'integration_single':  {
    'src' : ['integration' / 'integration_parallel_single.c'] + test_model_srcs,
    'rslib' : true,
    'mpi' : true
  },
  'integration_multi': {
    'src' : ['integration' / 'integration_parallel_multi.c'] + test_model_srcs,
    'rslib' : true,
    'mpi' : true
  },

  'mocked_pcs': {
    'src' : [
      'models' / 'model_test.c',
      models_dir / 'pcs' / 'pcs.c'
    ],
    'rslib' : true
  },
  'mocked_phold': {
    'src' : [
      'models' / 'model_test.c',
      models_dir / 'phold' / 'phold.c',
    ],
    'rslib' : true
  },

  'compiled_phold': {
    'src' : [
      models_dir / 'phold' / 'phold.c',
    ],
    'compile_model' : true,
  }
}

# Needed for mpi integration tests
if mpi_dep.found()
  mpiexec_cmd = find_program('mpiexec')
endif

# Tests builds
i = 1
foreach t_name, t_spec : test_specs
  t_full_name = 'test_' + i.to_string() + '_' + t_name
  t_srcs = t_spec.get('src')

  if t_spec.get('compile_model', false)
    test_executable = custom_target(t_full_name,
                                    output: t_full_name,
                                    input: t_srcs,
                                    command: [rootsim_cc, '@INPUT@',
                                              '-rsinc', src_dir,
                                              '-rslib', fs.parent(rootsim_lib.full_path()),
                                              '-o', '@OUTPUT@'],
                                    depends: rootsim_llvm_lib,
                                    build_always: true)
    test(t_full_name, model_runner, args: test_executable.full_path())
    continue
  endif

  if t_spec.get('run_plugin', false)
    t_srcs = [clang_with_plugin.process(t_srcs)]
  endif

  t_deps = []
  t_srcs += 'test' / 'test_framework.c'

  if t_spec.get('rslib', false)
    t_deps += rootsim_dep
  else
    t_srcs += 'test' / 'test_stubs.c'
    if not t_spec.get('has_main', false)
      t_srcs += 'test' / 'test_main.c'
      t_deps += rs_thr_dep
    endif
  endif

  test_executable = executable(t_full_name, t_srcs, dependencies : t_deps,
    include_directories : src_inc_dir, c_args : t_spec.get('c_args', []) +
    ['-DROOTSIM_TEST_NAME="' + t_name + '"'], link_args : '-no-pie',
    link_language : 'c')

  if mpi_dep.found() and t_spec.get('mpi', false)
    test(t_full_name + '_mpi', mpiexec_cmd,
      args : ['-n', '2', test_executable.full_path()])
  endif

  test(t_full_name, test_executable, is_parallel : false,
    should_fail : t_spec.get('fail', false))

  i = i + 1
endforeach
