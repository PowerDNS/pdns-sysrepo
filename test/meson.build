gtest_dep = dependency('gtest')
gmock_h = meson.get_compiler('cpp').has_header('gmock/gmock.h')
if not gmock_h
  error('gmock/gmock.h not found')
endif
gmock_dep = dependency('gmock')

# we need to rethink this..
src_include = include_directories('../src', '.', '..', '../sr_wrapper')

microhttp_dep = dependency('libmicrohttpd')
httpmockserver_include = include_directories('httpmockserver/include')

config_test_src = files('create-config/create-config.cc',
  '../src/configurator/configurator.cc',
  '../src/configurator/util.cc')

zones_test_src = files('zones/zone-add-delete.cc',
  '../src/configurator/subscribe.cc',
  '../src/configurator/subscribe/pdns-config.cc',
  '../src/configurator/subscribe/helpers.cc',
  '../src/configurator/subscribe/zones.cc',
  '../src/configurator/subscribe/service.cc',
  '../src/configurator/util.cc',
  '../src/config/config.cc',
  '../src/configurator/configurator.cc',
  '../sr_wrapper/session.cc'
  )

config_test = executable('create-config',
  sources: config_test_src,
  include_directories: [src_include],
  dependencies: [spdlog_dep, sysrepo_cpp_dep, libyang_cpp_dep, boost_deps, mstch_dep, gtest_dep])

yang_typedef_test_sources = files(
  'yang-typedefs/yang-typedefs.cc'
)

yang_typedef_test = executable('yang-typedefs',
  sources: yang_typedef_test_sources,
  dependencies: [libyang_cpp_dep, gtest_dep]
)
test('yang-typedefs', yang_typedef_test, args: [meson.source_root() + '/yang'])

if microhttp_dep.found()
  httpmockserver_dep = declare_dependency(sources: 'httpmockserver/src/mock_server.cc',
  dependencies: [microhttp_dep])

  zones_test = executable('zones-test',
  sources: zones_test_src,
  include_directories: [src_include, httpmockserver_include, pdns_api_cpp_incdir],
  dependencies: [spdlog_dep, sysrepo_cpp_dep, libyang_cpp_dep, boost_deps, mstch_dep, gtest_dep, httpmockserver_dep, pdns_api_cpp_dep, sdbusplus_dep])
  test('zones', zones_test, args: [meson.source_root() + '/yang'])
endif

test('configurator', config_test, args: [meson.source_root() + '/yang'])
subdir('remote-backend')