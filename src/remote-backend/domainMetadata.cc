/*
 * Copyright 2019-2020 Pieter Lexis <pieter.lexis@powerdns.com>
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
  libyang::S_Data_Node tree;
  try {
    tree = getZoneTree(session);
  } catch(const sysrepo::sysrepo_exception &e) {
    auto errors = getErrorsFromSession(session);
    spdlog::error("remote-backend - getDomainMetadata - {}", e.what());
    for (auto const &error : errors) {
      spdlog::error("remote-backend - getDomainMetadata -     xpath={} message=", error.first, error.second);
    }
    sendError(request, response, e.what());
    return;
  }

  auto zonesNodeXPath = fmt::format("/pdns-server:zones/zones[name='{}']", zone);
  auto zoneNode = session->get_subtree(zonesNodeXPath.c_str());

  try {
    if (kind == "ALSO-NOTIFY") {
      auto alsoNotifyXPath = fmt::format("/pdns-server:zones/also-notify", zone);
      for (auto const& alsoNotifyNode : zoneNode->find_path(alsoNotifyXPath.c_str())->data()) {
        auto alsoNotifyleaf = std::make_shared<libyang::Data_Node_Leaf_List>(alsoNotifyNode);
        auto notifyXPath = fmt::format("/pdns-server:notify-endpoint[name='{}']", alsoNotifyleaf->value_str());
        auto notifyNode = session->get_subtree(notifyXPath.c_str());
        for (auto const& notifyAddrNode : notifyNode->find_path("/pdns-server:notify-endpoint/address")->data()) {
          auto n = notifyAddrNode->child()->next(); // /pdns-server/notify-endpoint[name]/address[name]/ip-address
          auto addrLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(n);
          n = n->next(); // /pdns-server:notify-endpoint[name]/address[name]/port
          auto portLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(n);
          iputils::ComboAddress a(addrLeaf->value_str(), portLeaf->value()->uint16());
          metadata.push_back(a.toStringWithPort());
        }
      }
    }

    if (kind == "ALLOW-AXFR-FROM") {
      auto allowAxfrXPath = fmt::format("/pdns-server:zones/allow-axfr", zone);
      for (auto const& allowAxfrNode : zoneNode->find_path(allowAxfrXPath.c_str())->data()) {
        auto allowAxfrLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(allowAxfrNode);
        auto aclXPath = fmt::format("/pdns-server:axfr-access-control-list[name='{}']", allowAxfrLeaf->value_str());
        auto aclNode = session->get_subtree(aclXPath.c_str());
        for (auto const& aclAddressNode : aclNode->find_path("/pdns-server:axfr-access-control-list/network/ip-prefix")->data()) {
          auto aclAddressLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(aclAddressNode);
          metadata.push_back(aclAddressLeaf->value_str());
        }
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
