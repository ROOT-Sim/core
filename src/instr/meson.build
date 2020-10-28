generator_script = find_program('neurome_gen_exposed.py')

neurome_exposed_cpp = custom_target(
  'neurome_exposed_cpp',
  output : 'neurome_exposed.cpp',
  input : neurome_header,
  command : [generator_script, '@INPUT@', '@OUTPUT@']
)

# NeuRome LLVM plugin for model instrumentation
neurome_llvm_lib = shared_library('neurome-llvm', 
  ['instr_llvm.cpp', neurome_exposed_cpp], cpp_args : llvm_plugin_cpp_args, 
  include_directories : src_inc_dir, dependencies : llvm_dep, install : true)