sphinx = find_program('sphinx-build', required : false, disabler : true)

sphinx_conf = configure_file(input: 'conf.py.in', output : 'conf.py',
  configuration : {
    'DOC_STATIC_PATH': meson.current_source_dir() / 'static',
    'PROJECT_NAME': meson.project_name(),
    'PROJECT_VERSION': meson.project_version()
  }
)

sphinx_files = files(
  'first_steps.rst',
  'index.rst',
  'intro.rst',
  'installing.rst'
)

doc_static_files = files(
  'static' / 'logo_doc.png',
  'static' / 'logo.png',
  'static' / 'title.png'
)

doc_target = custom_target('sphinx', output : 'built',
  input : [sphinx_conf] + sphinx_files + doc_static_files, 
  command : [
    sphinx, 
    meson.current_source_dir(), 
    meson.current_build_dir() / 'built',
    '-c', meson.current_build_dir()
  ]
)

run_target('doc', command : '', depends : doc_target)

# mainpage = vcs_tag(command : ['git', 'log', '-1', '--format=%h'],
#		 fallback : 'unknown',
#		 input : 'mainpage.dox',
#		 output : 'mainpage.dox',
#		 replace_string: '__GIT_VERSION__')

# doc_config = configuration_data()
# doc_config.set('PACKAGE_NAME', )
# doc_config.set('PACKAGE_VERSION', meson.project_version())
# doc_config.set('builddir', meson.current_build_dir())

# doxyfile = configure_file(input : 'libinput.doxygen.in',
#			  output : 'libinput.doxygen',
#			  configuration : doc_config,
#			  install : false)
