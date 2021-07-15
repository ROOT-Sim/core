# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

# Model helper library sources
lib_srcs = files(
  'random' / 'random.c',
  'state' / 'state.c',
  'topology' / 'topology.c',
  'lib.c'
)

lib_objs = clang_with_plugin.process(lib_srcs)
lib_objs_fpic = clang_with_plugin.process(lib_srcs, 
  extra_args : ['-fPIC', '-fvisibility=hidden'])

static_library('rootsim-mods', lib_objs, link_language : 'c',
  gnu_symbol_visibility : 'hidden', install : true)
rootsim_mods_lib = shared_library('rootsim-mods', lib_objs_fpic, link_language : 'c',
  gnu_symbol_visibility : 'hidden', install : true)
