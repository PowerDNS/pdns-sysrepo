project('pdns-api-cpp', 'cpp',
  version : '0.0.14',
  default_options : ['cpp_std=c++11'],
  license: ['MIT', 'GLPv2']
  )

boost_deps = dependency('boost', modules : ['algorithm', 'optional', 'uuid'])

cpprest_dep = dependency('cpprestsdk', required: false)
if not cpprest_dep.found()
  cpprest_dep = dependency('cpprest', required: false)
endif
if not cpprest_dep.found()
  cpprest_dep = meson.get_compiler('cpp').find_library('libcpprest')
endif

openssl_dep = dependency('openssl')

pdns_api_sources = files(
'include/ApiClient.cpp',
'include/ApiConfiguration.cpp',
'include/ApiException.cpp',
'include/HttpContent.cpp',
'include/JsonBody.cpp',
'include/ModelBase.cpp',
'include/MultipartFormData.cpp',
'include/Object.cpp',
'include/api/ConfigApi.cpp',
'include/api/SearchApi.cpp',
'include/api/ServersApi.cpp',
'include/api/TsigkeyApi.cpp',
'include/api/ZonecryptokeyApi.cpp',
'include/api/ZonemetadataApi.cpp',
'include/api/ZonesApi.cpp',
'include/model/CacheFlushResult.cpp',
'include/model/Comment.cpp',
'include/model/ConfigSetting.cpp',
'include/model/Cryptokey.cpp',
'include/model/Error.cpp',
'include/model/MapStatisticItem.cpp',
'include/model/Metadata.cpp',
'include/model/RRSet.cpp',
'include/model/Record.cpp',
'include/model/RingStatisticItem.cpp',
'include/model/SearchResult.cpp',
'include/model/SearchResultComment.cpp',
'include/model/SearchResultComment.h',
'include/model/SearchResultRecord.cpp',
'include/model/SearchResultZone.cpp',
'include/model/SearchResults.cpp',
'include/model/Server.cpp',
'include/model/SimpleStatisticItem.cpp',
'include/model/StatisticItem.cpp',
'include/model/TSIGKey.cpp',
'include/model/Zone.cpp')

pdns_api_cpp_incdir = include_directories('include', 'include/model', 'include/api')

pdns_api_cpp_dep = declare_dependency(
  dependencies: [boost_deps, cpprest_dep, openssl_dep],
  include_directories: pdns_api_cpp_incdir,
  sources: pdns_api_sources,
  version: meson.project_version()
)