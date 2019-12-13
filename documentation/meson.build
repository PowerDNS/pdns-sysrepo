doc_files = files(
  'changelog.rst',
  'development/building.rst',
  'development/index.rst',
  'development/release.rst',
  'development/sysrepo-gotchas.rst',
  'guides/config-changes.rst',
  'guides/config.rst',
  'index.rst',
  'install.rst'
)

custom_target('documentation',
  input: doc_files,
  output: 'documentation',
  command : [sphinxbuild,
            '-q',
            '-b', 'html',
            meson.current_source_dir(),
            '@OUTPUT@'],
  install : true,
  install_dir : get_option('prefix') + '/' + get_option('datadir') + '/pdns-sysrepo')