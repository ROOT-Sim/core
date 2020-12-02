llvm_cfg_gen = find_program('llvm_cfg_gen.py')

instr_cfg = custom_target(
  'instr_cfg',
  output : 'instr_cfg.cpp',
  input : [],
  command : [llvm_cfg_gen, '@OUTPUT@']
)

# ROOT-Sim LLVM plugin for model instrumentation
rootsim_llvm_lib = shared_library('rootsim-llvm', ['instr_llvm.cpp', instr_cfg],
  dependencies : llvm_dep, name_prefix : 'lib', name_suffix : 'so',
  cpp_args : log_level_def, include_directories : src_inc_dir, install : true)

clang_arguments = ['-O' + get_option('optimization')]

if get_option('debug')
  clang_arguments += '-g'
endif

clang_arguments += ['-I', meson.source_root() / 'src', '@INPUT@', '-c', 
  '-o', '@OUTPUT@', '-Xclang', '-load', '-Xclang', rootsim_llvm_lib.full_path()]

clang_with_plugin = generator(clang_compiler, output : '@BASENAME@.o', 
  arguments : clang_arguments, depends : rootsim_llvm_lib)