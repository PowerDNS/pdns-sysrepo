gtest_dep = dependency('gtest')
gmock_dep = dependency('gmock')

src_include = include_directories('../src')

config_test_src = files('create-config/create-config.cc',
  '../src/configurator/configurator.cc',
  '../src/configurator/util.cc')

config_test = executable('create-config',
  sources: config_test_src,
  include_directories: [src_include],
  dependencies: [spdlog_dep, sysrepo_cpp_dep, libyang_cpp_dep, boost_deps, mstch_dep, gtest_dep])

test('configurator', config_test)