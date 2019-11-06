project('pdns-sysrepo', 'cpp',
  version : '0.0.0',
  default_options : ['cpp_std=c++17'],
  license: ['MIT', 'Apache2']
  )

conf_data = configuration_data()
conf_data.set('prefix', get_option('prefix'))
conf_data.set('bindir', get_option('bindir'))
conf_data.set('version', meson.project_version())

sr_wrapper = files('sr_wrapper/session.hh', 'sr_wrapper/session.cc')
pdns_configurator = files('src/configurator/configurator.cc', 'src/configurator/config.cc',
  'src/configurator/subscribe.hh', 'src/configurator/subscribe.cc',
  'src/configurator/util.hh', 'src/configurator/util.cc'
  )

sysrepo_cpp_dep = dependency('libSysrepo-cpp', version: '>=1.2.0')
libyang_cpp_dep = dependency('libyang-cpp')
spdlog_dep = dependency('spdlog')
boost_deps = dependency('boost', modules : ['filesystem', 'program_options'])
mstch_dep = dependency('mstch')
yaml_dep = dependency('yaml-cpp')
libsystemd_dep = dependency('libsystemd')

## This will need to be shipped to the customer though...
sdbusplus_dep = dependency('sdbusplus')

version_h = configure_file(input : 'version.h.in',
               output : 'version.h',
               configuration : conf_data)

executable('pdns-sysrepo-configurator', 'src/pdns-configurator.cc',
  sr_wrapper, pdns_configurator, version_h,
  dependencies: [sysrepo_cpp_dep, spdlog_dep, boost_deps, mstch_dep, yaml_dep, libsystemd_dep, libyang_cpp_dep, sdbusplus_dep],
  install: true)

# Generate a systemd unit file
systemd_dep = dependency('systemd')
configure_file(input : 'pdns-configurator.service.in',
               output : 'pdns-configurator.service',
               configuration : conf_data,
               install: true,
               install_dir: systemd_dep.get_pkgconfig_variable('systemdsystemunitdir'))

install_data('polkit/10-pdns.rules', install_dir: get_option('datadir') + '/polkit-1/rules.d')