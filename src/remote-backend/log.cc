/**
 * Copyright Pieter Lexis <pieter.lexis@powerdns.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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