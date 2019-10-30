project('pdns-sysrepo', 'cpp',
  version : '0.0.0',
  default_options : ['cpp_std=c++11'],
  license: ['MIT', 'Apache2']
  )

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
systemd_dep = dependency('libsystemd')

executable('pdns-sysrepo-configurator', 'src/pdns-configurator.cc',
  sr_wrapper, pdns_configurator,
  dependencies: [sysrepo_cpp_dep, spdlog_dep, boost_deps, mstch_dep, yaml_dep, systemd_dep, libyang_cpp_dep])
