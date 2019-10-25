project('pdns-sysrepo', 'cpp',
  version : '0.0.0',
  default_options : ['cpp_std=c++11'],
  license: ['MIT', 'Apache2']
  )

sr_wrapper = files('sr_wrapper/session.hh', 'sr_wrapper/session.cc')
sysrepo_cpp_dep = dependency('libSysrepo-cpp', version: '>=1.2.0')
spdlog_dep = dependency('spdlog')

executable('pdns-sysrepo-configurator', 'src/pdns-configurator.cc',
  sr_wrapper,
  dependencies: [sysrepo_cpp_dep, spdlog_dep])
