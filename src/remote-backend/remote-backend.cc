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

namespace pdns_sysrepo::remote_backend
{
RemoteBackend::RemoteBackend(sysrepo::S_Connection& connection, const Pistache::Address& addr) :
  d_connection(connection),
  d_endpoint(std::make_shared<Http::Endpoint>(addr)) {
  Rest::Routes::Get(d_router, "/dns/lookup/:recordname/:type", Rest::Routes::bind(&RemoteBackend::lookup, this));
  Rest::Routes::Get(d_router, "/dns/getAllDomains", Rest::Routes::bind(&RemoteBackend::getAllDomains, this));
  Rest::Routes::Get(d_router, "/dns/list/:id/:zone", Rest::Routes::bind(&RemoteBackend::list, this));
  Rest::Routes::Get(d_router, "/dns/getUpdatedMasters", Rest::Routes::bind(&RemoteBackend::getUpdatedMasters, this));
  Rest::Routes::Patch(d_router, "/dns/setNotified/:id", Rest::Routes::bind(&RemoteBackend::setNotified, this));
  Rest::Routes::Get(d_router, "/dns/getDomainMetadata/:zone/:kind", Rest::Routes::bind(&RemoteBackend::getDomainMetadata, this));
  Rest::Routes::Get(d_router, "/dns/getUnfreshSlaveInfos", Rest::Routes::bind(&RemoteBackend::getUnfreshSlaveInfos, this));
  Rest::Routes::NotFound(d_router, Rest::Routes::bind(&RemoteBackend::notFound, this));

  auto opts = Http::Endpoint::options().threads(4).flags(Pistache::Tcp::Options::ReuseAddr);
  d_endpoint->init(opts);
};

RemoteBackend::~RemoteBackend() {
  try {
    d_endpoint->shutdown();
  }
  catch (const std::exception& e) {
    spdlog::warn("Exception in RemoteBackend destructor: {}", e.what());
  }
};

void RemoteBackend::start() {
  d_endpoint->setHandler(d_router.handler());
  // This ensures we actually return
  d_endpoint->serveThreaded();
}
} // namespace pdns_sysrepo::remote_backend