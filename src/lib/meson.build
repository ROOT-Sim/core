# Model helper library sources
lib_srcs = files(
  'random' / 'random.c',
  'state' / 'state.c',
  'topology' / 'topology.c',
  'lib.c'
)

lib_objs = clang_with_plugin.process(lib_srcs)

rootsim_mods_lib = static_library('rootsim-mods', lib_objs, install : true)
