generator_script = find_program('llvm_cfg_gen.py')

instr_cfg_cpp = custom_target(
  'instr_cfg',
  output : 'instr_cfg.cpp',
  input : rootsim_header,
  command : [generator_script, '@INPUT@', '@OUTPUT@']
)

# ROOT-Sim LLVM plugin for model instrumentation
rootsim_llvm_lib = shared_library('rootsim-llvm', 
  ['instr_llvm.cpp', instr_cfg_cpp], cpp_args : llvm_plugin_cpp_args, 
  include_directories : src_inc_dir, dependencies : llvm_dep, install : true)
