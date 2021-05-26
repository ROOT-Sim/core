# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

# Model helper library sources
lib_srcs = files(
  'random' / 'random.c',
  'state' / 'state.c',
  'topology' / 'topology.c',
  'config' / 'jsmn.c',
  'config' / 'reflect.c',
  'lib.c'
)

lib_objs = clang_with_plugin.process(lib_srcs)

rootsim_mods_lib = static_library('rootsim-mods', lib_objs, install : true)
