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
  command : [sphinxbuild,
            '-b', 'html',
            meson.current_source_dir(),
            meson.current_build_dir()],
  input: doc_files,
  output: 'documentation',
  install : true,
  install_dir : get_option('prefix') + '/' + get_option('datadir') + '/pdns-sysrepo')