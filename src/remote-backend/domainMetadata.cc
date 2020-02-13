/*
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
#include "iputils/iputils.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::getDomainMetadata(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
  static const std::set<string> allowedMetadata = {"ALSO-NOTIFY", "ALLOW-AXFR-FROM"};
  logRequest(request);
  auto zone = request.param(":zone").as<string>();
  zone = urlDecode(zone);
  auto kind = request.param(":kind").as<string>();
  kind = urlDecode(kind);
  spdlog::debug("RemoteBackend::{} - Requested metadata - zone={} kind={}", __func__, zone, kind);

  try {
    auto bestZone = findBestZone(zone);
    if (bestZone != zone) {
      sendError(request, response, fmt::format("No such zone {}", zone));
      return;
    }
  }
  catch (const std::out_of_range& e) {
    sendError(request, response, e.what());
    return;
  }

  if (allowedMetadata.find(kind) == allowedMetadata.end()) {
    sendError(request, response, fmt::format("Metadata type {} not allowed", kind));
    return;
  }

  nlohmann::json::array_t metadata;
  auto session = getSession();
  libyang::S_Data_Node zoneTree, pdnsServerTree;
  try {
    zoneTree = getZoneTree(session);
    pdnsServerTree = session->get_subtree("/pdns-server:pdns-server");
  } catch(const sysrepo::sysrepo_exception &e) {
    auto errors = getErrorsFromSession(session);
    spdlog::error("RemoteBackend::getDomainMetadata - {}", e.what());
    logSessionErrors(session, spdlog::level::err);
    sendError(request, response, e.what());
    return;
  }

  try {
    if (kind == "ALSO-NOTIFY") {
      auto alsoNotifyXPath = fmt::format("/pdns-server:zones/zones[name='{}']/also-notify", zone);
      const static std::string globalAlsoNotifyXPath = "/pdns-server:pdns-server/pdns-server:also-notify";

      spdlog::trace("RemoteBackend::{} - Grabbing also-notify addresses - alsoNotifyXPath='{}' globalAlsoNotifyXPath='{}'", __func__, alsoNotifyXPath, globalAlsoNotifyXPath);
      for (auto const& alsoNotifyNode : zoneTree->find_path(alsoNotifyXPath.c_str())->data()) {
        auto addresses = getListofIPs(session, alsoNotifyNode, IpLeafRef::also_notify);
        metadata.insert(metadata.end(), addresses.begin(), addresses.end());
        break;
      }

      for (auto const& alsoNotifyNode : pdnsServerTree->find_path(globalAlsoNotifyXPath.c_str())->data()) {
        auto addresses = getListofIPs(session, alsoNotifyNode, IpLeafRef::also_notify);
        metadata.insert(metadata.end(), addresses.begin(), addresses.end());
        break;
      }
    }

    if (kind == "ALLOW-AXFR-FROM") {
      auto allowAxfrXPath = fmt::format("/pdns-server:zones/zones[name='{}']/allow-axfr", zone);
      const static std::string globalAllowAxfrXPath = "/pdns-server:pdns-server/pdns-server:allow-axfr";

      spdlog::trace("RemoteBackend::{} - Grabbing AXFR ACLs - allowAxfrXPath='{}' globalAllowAxfrPath='{}'", __func__, allowAxfrXPath, globalAllowAxfrXPath);
      for (auto const& allowAxfrNode : zoneTree->find_path(allowAxfrXPath.c_str())->data()) {
        auto masks = getListofNetmasks(session, allowAxfrNode, NetmaskLeafRef::allow_axfr);
        metadata.insert(metadata.end(), masks.begin(), masks.end());
        break;
      }

      for (auto const& allowAxfrNode : pdnsServerTree->find_path(globalAllowAxfrXPath.c_str())->data()) {
        auto masks = getListofNetmasks(session, allowAxfrNode, NetmaskLeafRef::allow_axfr);
        metadata.insert(metadata.end(), masks.begin(), masks.end());
        break;
      }
    }
  }
  catch (const sysrepo::sysrepo_exception& e) {
    auto errors = getErrorsFromSession(session);
    spdlog::error("remote-backend - getDomainMetadata - {} zone={} metadata_kind={}", e.what(), zone, kind);
    for (auto const& error : errors) {
      spdlog::error("remote-backend - getDomainMetadata -     xpath={} message=", error.first, error.second);
    }
    sendError(request, response, e.what());
    return;
  }

  sendResponse(request, response, nlohmann::json({{"result", metadata}}));
}
} // namespace pdns_sysrepo::remote_backend
