project('pdns-sysrepo', 'cpp',
  version : '0.0.0',
  default_options : ['cpp_std=c++17'],
  license: ['MIT', 'Apache2']
  )

conf_data = configuration_data()
conf_data.set('prefix', get_option('prefix'))
conf_data.set('bindir', get_option('bindir'))
conf_data.set('sysconfdir', get_option('sysconfdir'))
conf_data.set('version', meson.project_version())

sr_wrapper = files('sr_wrapper/session.hh', 'sr_wrapper/session.cc')
pdns_sysrepo = files('src/configurator/configurator.cc', 'src/configurator/config.cc',
  'src/configurator/subscribe.hh', 'src/configurator/subscribe.cc',
  'src/configurator/util.hh', 'src/configurator/util.cc'
  )

sysrepo_cpp_dep = dependency('libSysrepo-cpp', version: '>=1.2.0')
libyang_cpp_dep = dependency('libyang-cpp')
boost_deps = dependency('boost', modules : ['filesystem', 'program_options', 'system'])
yaml_dep = dependency('yaml-cpp')
libsystemd_dep = dependency('libsystemd')
## This will need to be shipped to the customer though...
sdbusplus_dep = dependency('sdbusplus')

# Ubuntu Bionic ships with an spdlog that does not support the API we use
spdlog_dep = dependency('spdlog', required: false)
if not spdlog_dep.found() or spdlog_dep.version().version_compare('<1.4.0')
  spdlog_proj = subproject('spdlog-1.4.2', default_options: ['enable_examples=false'])
  spdlog_dep = spdlog_proj.get_variable('spdlog_dep')
endif

fmt_dep = dependency('fmt', required: false)
if not fmt_dep.found()
  fmt_dep = meson.get_compiler('cpp').find_library('fmt')
endif

mstch_dep = dependency('mstch', required: false)
if not mstch_dep.found()
  mstch_dep = meson.get_compiler('cpp').find_library('mstch')
endif

version_h = configure_file(input : 'src/config.h.in',
               output : 'config.h',
               configuration : conf_data)

executable('pdns-sysrepo', 'src/pdns-sysrepo.cc',
  sr_wrapper, pdns_sysrepo, version_h,
  dependencies: [sysrepo_cpp_dep, spdlog_dep, boost_deps, mstch_dep, yaml_dep, libsystemd_dep, libyang_cpp_dep, sdbusplus_dep, fmt_dep],
  install: true)

# Generate a systemd unit file
systemd_dep = dependency('systemd')
configure_file(input : 'pdns-sysrepo.service.in',
               output : 'pdns-sysrepo.service',
               configuration : conf_data,
               install: true,
               install_dir: systemd_dep.get_pkgconfig_variable('systemdsystemunitdir'))

install_data('polkit/10-pdns.rules', install_dir: get_option('datadir') + '/polkit-1/rules.d')
install_data('pdns-sysrepo.example.yaml', install_dir: get_option('sysconfdir') + '/pdns-sysrepo')
install_data('yang/pdns-server.yang', 'yang/ietf-inet-types@2013-07-15.yang',
              install_dir: get_option('datadir') + '/pdns-sysrepo')