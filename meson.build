project('pdns-sysrepo', 'cpp',
  version : '0.3.2',
  default_options : ['cpp_std=c++17'],
  license: ['Apache2']
  )

conf_data = configuration_data()
conf_data.set('prefix', get_option('prefix'))
conf_data.set('bindir', get_option('bindir'))
conf_data.set('sysconfdir', get_option('sysconfdir'))
conf_data.set('version', meson.project_version())

sr_wrapper_sources = files('sr_wrapper/session.cc')

pdns_sysrepo_sources = files(
  'src/pdns-sysrepo.cc',
  'src/config/config.cc'
  )

pdns_sysrepo_util_sources = files(
  'src/util/util.cc',
)

pdns_sysrepo_remote_backend_sources = files(
  'src/remote-backend/list.cc', 
  'src/remote-backend/lookup.cc',
  'src/remote-backend/httpfunc.cc',
  'src/remote-backend/getAllDomains.cc',
  'src/remote-backend/json-helpers.cc',
  'src/remote-backend/find-zone.cc',
  'src/remote-backend/sysrepo-helpers.cc',
  'src/remote-backend/getUpdatedMasters.cc',
  'src/remote-backend/domain-id.cc',
  'src/remote-backend/setNotified.cc',
  'src/remote-backend/log.cc',
  'src/remote-backend/remote-backend.cc',
  'src/remote-backend/domainMetadata.cc',
  'src/remote-backend/getUnfreshSlaveInfos.cc',
  'src/remote-backend/zone-helpers.cc',
  'src/remote-backend/getDomainInfo.cc',
  'src/remote-backend/setFresh.cc',
  'src/remote-backend/transactions.cc',
  'src/remote-backend/feedRecord.cc'
)

pdns_sysrepo_pdns_config_sources = files(
  'src/pdns-config/api-client.cc',
  'src/pdns-config/config-update.cc',
  'src/pdns-config/pdns-config-callback.cc',
  'src/pdns-config/pdns-config.cc',
  'src/pdns-config/service.cc',
  'src/pdns-config/zone-management.cc',
)

pdns_sysrepo_zone_oper_callback = files(
  'src/zone-oper-callback/zone-oper-callback.cc'
)

sysrepo_cpp_dep = dependency('sysrepo-cpp', version: '>=1.4.26')
libyang_cpp_dep = dependency('libyang-cpp', version: '>=1.0.146')
boost_deps = dependency('boost', modules : ['filesystem', 'program_options', 'system'])
libsystemd_dep = dependency('libsystemd')
sdbusplus_dep = dependency('sdbusplus')

# Ubuntu Bionic ships with an spdlog that does not support the API we use
# So let's just use the spdlog we ship anyway
spdlog_proj = subproject('spdlog-1.4.2', default_options: ['enable_examples=false'])
spdlog_dep = spdlog_proj.get_variable('spdlog_dep')

json_proj = subproject('nlohmann-json')
json_dep = json_proj.get_variable('nlohmann_json_dep')

mstch_dep = dependency('mstch', required: false)
if not mstch_dep.found()
  mstch_dep = meson.get_compiler('cpp').find_library('mstch')
endif

# pdns_api_cpp_incdir =  include_directories('ext', 'ext/pdns-api-cpp/model', 'ext/pdns-api-cpp/api')
pdns_api_cpp_proj = subproject('pdns-api-cpp')
pdns_api_cpp_dep = pdns_api_cpp_proj.get_variable('pdns_api_cpp_dep')
pdns_api_cpp_incdir = pdns_api_cpp_proj.get_variable('pdns_api_cpp_incdir')

pistache_sources = files(
'subprojects/pistache/src/server/endpoint.cc',
'subprojects/pistache/src/server/router.cc',
'subprojects/pistache/src/server/listener.cc',
'subprojects/pistache/src/common/description.cc',
'subprojects/pistache/src/common/timer_pool.cc',
'subprojects/pistache/src/common/utils.cc',
'subprojects/pistache/src/common/os.cc',
'subprojects/pistache/src/common/transport.cc',
'subprojects/pistache/src/common/net.cc',
'subprojects/pistache/src/common/reactor.cc',
'subprojects/pistache/src/common/mime.cc',
'subprojects/pistache/src/common/http_headers.cc',
'subprojects/pistache/src/common/stream.cc',
'subprojects/pistache/src/common/http.cc',
'subprojects/pistache/src/common/tcp.cc',
'subprojects/pistache/src/common/http_defs.cc',
'subprojects/pistache/src/common/peer.cc',
'subprojects/pistache/src/common/http_header.cc',
'subprojects/pistache/src/common/cookie.cc')

pistache_include = include_directories('subprojects/pistache/include')

pistache_dep = declare_dependency(sources: pistache_sources,
  include_directories: [pistache_include])

version_h = configure_file(input : 'src/config.h.in',
               output : 'config.h',
               configuration : conf_data)

pdns_sysrepo_bin = executable('pdns-sysrepo',
  pdns_sysrepo_pdns_config_sources,
  pdns_sysrepo_remote_backend_sources,
  pdns_sysrepo_sources,
  pdns_sysrepo_util_sources,
  pdns_sysrepo_zone_oper_callback,
  sr_wrapper_sources,
  version_h,
  dependencies: [sysrepo_cpp_dep, spdlog_dep, boost_deps, mstch_dep, libsystemd_dep, libyang_cpp_dep, sdbusplus_dep, pdns_api_cpp_dep, pistache_dep, json_dep],
  include_directories: [pdns_api_cpp_incdir, include_directories('src'), pistache_include],
  install: true)

# Generate a systemd unit file
systemd_dep = dependency('systemd')
configure_file(input : 'pdns-sysrepo.service.in',
               output : 'pdns-sysrepo.service',
               configuration : conf_data,
               install: true,
               install_dir: systemd_dep.get_pkgconfig_variable('systemdsystemunitdir'))

install_data('polkit/10-pdns.rules', install_dir: get_option('datadir') + '/polkit-1/rules.d')
install_data('yang/pdns-server.yang', 'yang/ietf-inet-types@2013-07-15.yang', 'yang/iana-dns-class-rr-type@2019-06-27.yang',
             'yang/iana-dns-parameters@2016-08-03.yang', 'yang/dns-zone-common@2017-01-12.yang', 'yang/ietf-yang-types@2013-07-15.yang',
              install_dir: get_option('datadir') + '/pdns-sysrepo')

##
# Documentation
##
sphinxbuild = find_program('sphinx-build', required: false)
subdir('documentation', if_found: sphinxbuild)
if not sphinxbuild.found()
  message('sphinx-build not found. Unable to generate documentation')
endif

##
# Tests
##
if get_option('build-tests')
  subdir('test')
endif
