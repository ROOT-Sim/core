doxygen = find_program('doxygen', required : false, disabler: true)

cdata = configuration_data()
cdata.set('VERSION', meson.project_version())
cdata.set('TOP_SRCDIR', meson.source_root())
cdata.set('TOP_BUILDDIR', meson.build_root())

if find_program('dot', required : false).found()
  cdata.set('HAVE_DOT', 'YES')
else
  cdata.set('HAVE_DOT', 'NO')
endif

fs = import('fs')

doxyfile = configure_file(input: 'Doxyfile.in',
                          output: 'Doxyfile',
                          configuration: cdata,
                          install: false)

# WTF meson! So much hassle for such a stupid and common thing!
docs_files = [
  'flatdoc.css',
  'inline-params.css',
  'img/rs.png'
]
foreach f: docs_files
    configure_file(input: f, output: '@0@'.format(fs.name(f)), copy: true)
endforeach

html_target = custom_target('htmldocs',
                            build_by_default: false,
                            input: doxyfile,
                            output: 'html',
                            command: [doxygen, doxyfile])

run_target('docs', command : '', depends : html_target)

coverage = configure_file(input: 'coverage.sh.in',
                          output : 'coverage.sh',
                          configuration : cdata)

run_target('docs-coverage',
           command: coverage,
           depends: html_target)
