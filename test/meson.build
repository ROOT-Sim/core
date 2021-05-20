# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

src_dir = meson.source_root() / 'src'
models_dir = meson.source_root() / 'models'
# Test dependencies
rootsim_dep = declare_dependency(link_with : [rootsim_lib, rootsim_mods_lib])

test_model_srcs = [
  'integration' / 'model' / 'application.c',
  'integration' / 'model' / 'functions.c',
  'integration' / 'model' / 'output_256.c'
]

test_specs = {
  'fail_init': {'src' : ['test' / 'fail_init.c'], 'fail' : true},
  'fail_body': {'src' : ['test' / 'fail_body.c'], 'fail' : true},
  'fail_fini': {'src' : ['test' / 'fail_fini.c'], 'fail' : true},
  'fail_main': {'src' : ['test' / 'fail_main.c'], 'fail' : true, 'has_main' : true},

  'unit_init' : {'src' : ['core' / 'init_test.c', src_dir / 'core' / 'init.c', src_dir / 'core' / 'arg_parse.c'], 'has_main' : true},
  'unit_sync' : {'src' : ['core' / 'sync_test.c', src_dir / 'core' / 'sync.c']},
  'unit_msg_queue' : {'src' : ['datatypes' / 'msg_queue_test.c', src_dir / 'datatypes' / 'msg_queue.c']},
# 'unit_gvt' : {'src' : ['gvt' / 'gvt_test.c', src_dir / 'gvt' / 'gvt.c']},
  'unit_msg_allocator' : {'src' : ['mm' / 'msg_allocator_test.c', src_dir / 'mm' / 'msg_allocator.c']},
  'unit_lp' : {'src' : ['lp' / 'lp_test.c', src_dir / 'lp' / 'lp.c']},
  'unit_buddy_allocator' : {'src' : ['mm' / 'buddy' / 'buddy_test.c', src_dir / 'mm' / 'buddy' / 'buddy.c']},
  'unit_compiler' : {'src' : ['compiler' / 'compiler_test.c', src_dir / 'compiler' / 'compiler.c'], 'has_main' : true, 'c_args' : compiler_c_args},

  'integration_serial': {
    'src' : ['integration' / 'integration_serial.c'] + test_model_srcs,
    'run_plugin' : true,
    'rslib' : true
  },
  'integration_single':  {
    'src' : ['integration' / 'integration_parallel_single.c'] + test_model_srcs,
    'run_plugin' : true,
    'rslib' : true,
    'mpi' : true
  },
  'integration_multi': {
    'src' : ['integration' / 'integration_parallel_multi.c'] + test_model_srcs,
    'run_plugin' : true,
    'rslib' : true,
    'mpi' : true
  },

  'model_pcs': {
    'src' : [
      'models' / 'model_test.c',
      models_dir / 'pcs' / 'pcs.c',
    ],
    'run_plugin' : true,
    'rslib' : true
  },
  'model_phold': {
    'src' : [
      'models' / 'model_test.c',
      models_dir / 'phold' / 'phold.c',
    ],
    'run_plugin' : true,
    'rslib' : true
  }
}

# Needed for mpi integration tests
if mpi_dep.found()
  mpiexec_cmd = find_program('mpiexec')
endif

# Tests builds
i = 1
foreach t_name, t_spec : test_specs
  t_srcs = t_spec.get('src')
  if t_spec.get('run_plugin', false)
    t_srcs = [clang_with_plugin.process(t_srcs)]
  endif

  t_deps = []
  t_srcs += 'test.c'

  if t_spec.get('rslib', false)
    t_deps += rootsim_dep
  else
    t_srcs += src_dir / 'mm' / 'mm.c'
    if not t_spec.get('has_main', false)
      t_srcs += 'test_main.c'
      t_deps += rs_thr_dep
    endif
  endif

  t_full_name = 'test_' + i.to_string() + '_' + t_name

  test_executable = executable(t_full_name, t_srcs, dependencies : t_deps,
    include_directories : src_inc_dir, c_args : t_spec.get('c_args', []) +
    ['-DROOTSIM_TEST_NAME="' + t_name + '"'], link_args : '-no-pie')

  if mpi_dep.found() and t_spec.get('mpi', false)
    test(t_full_name + '_mpi', mpiexec_cmd,
      args : ['-n', '2', test_executable.full_path()])
  endif

  test(t_full_name, test_executable, is_parallel : false,
    should_fail : t_spec.get('fail', false))

  i = i + 1
endforeach
