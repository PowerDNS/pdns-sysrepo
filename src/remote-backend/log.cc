#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend {
  void RemoteBackend::logRequest(const Pistache::Rest::Request &request){
    spdlog::debug("remote-backend - Had request - method={} resource={} remote={}:{}",
      Pistache::Http::methodString(request.method()), request.resource(), request.address().host(), request.address().port());
  }

  void RemoteBackend::logRequestResponse(const Pistache::Rest::Request &request, const Pistache::Http::Response &response, const nlohmann::json& ret){
    spdlog::info("remote-backend - Finished request - address={} method={} resource={} http_version={} result_code={} result_size={}",
      request.address().host(), Pistache::Http::methodString(request.method()), request.resource(), Pistache::Http::versionString(request.version()),
      response.code(), ret.dump().size());
    spdlog::debug("remote-backend - Finished request - result_body='{}'", ret.dump());
  }
}