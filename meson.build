project('ROOT-Sim', ['c', 'cpp'], version: '3.0.0', 
  default_options: ['c_std=c17', 'cpp_std=c++14', 'buildtype=release', 
  'warning_level=3', 'unity=on', 'unity_size=10000'])
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c')
subdir('src')
subdir('test')
subdir('doc')
