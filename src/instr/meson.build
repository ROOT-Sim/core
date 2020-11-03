generator_script = find_program('rootsim_gen_exposed.py')

rootsim_exposed_cpp = custom_target(
  'rootsim_exposed_cpp',
  output : 'rootsim_exposed.cpp',
  input : rootsim_header,
  command : [generator_script, '@INPUT@', '@OUTPUT@']
)

# ROOT-Sim LLVM plugin for model instrumentation
rootsim_llvm_lib = shared_library('rootsim-llvm', 
  ['instr_llvm.cpp', rootsim_exposed_cpp], cpp_args : llvm_plugin_cpp_args, 
  include_directories : src_inc_dir, dependencies : llvm_dep, install : true)
