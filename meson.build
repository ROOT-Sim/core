project('ROOT-Sim', ['c', 'cpp'], version: '3.0.0', 
  default_options: ['c_std=c11', 'cpp_std=c++14', 'buildtype=release', 
  'warning_level=3', 'unity=on', 'unity_size=10000'])

# Include directory used explicitly in tests and llvm plugin compilation
src_inc_dir = include_directories('src')

# Math library dependency
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

# Threads library dependency
rs_thr_dep = declare_dependency(link_with: 
  static_library('rootsim-threads', 'src' / 'arch' / 'arch.c', 
    dependencies: dependency('threads'), include_directories : src_inc_dir))

subdir('src')
subdir('test')
subdir('docs')
