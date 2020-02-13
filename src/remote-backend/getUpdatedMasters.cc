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
void RemoteBackend::getUpdatedMasters(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
  logRequest(request);
  auto session = getSession();
  libyang::S_Data_Node zonesNode;
  try {
    zonesNode = getZoneTree(session);
    zonesNode = zonesNode->child();
    if (!zonesNode) {
      throw std::out_of_range("No zones defined");
    }
  } catch(const std::exception& e) {
    sendResponse(request, response, nlohmann::json({{"result", false}, {"log", nlohmann::json::array({e.what()})}}));
    return;
  }

  nlohmann::json::array_t allZones;
  for (auto const& zone : zonesNode->tree_for()) {
    auto zoneNode = zone->child(); //  This is /pdns-server:zones/zones[name]/name
    nlohmann::json domainInfo;
    string zoneName;

    {
      auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(zoneNode);
      zoneName = leaf->value_str();
    }

    auto zonetypeXPath = fmt::format("/pdns-server:zones/zones[name='{}']/zonetype", zoneName);
    auto zonetypeLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(zone->find_path(zonetypeXPath.c_str())->data().at(0));
    spdlog::trace("RemoteBackend::getUpdatedMasters - zonetypeLeaf path={} value={}", zonetypeLeaf->path(), zonetypeLeaf->value_str());
    if (std::string(zonetypeLeaf->value_str()) != "master") {
      continue;
    }

    domainInfo["zone"] = zoneName;

    auto domainId = getDomainID(zoneName);
    domainInfo["id"] = domainId;

    auto serialXPath = fmt::format("/pdns-server:zones/zones[name='{}']/rrset[owner='{}'][type='SOA']/rdata/SOA/serial", zoneName, zoneName);
    auto zoneSOASerialNode = zone->find_path(serialXPath.c_str())->data().at(0);
    auto zoneSOASerial = std::make_shared<libyang::Data_Node_Leaf_List>(zoneSOASerialNode)->value()->uint32();
    domainInfo["serial"] = zoneSOASerial;

    uint32_t notifiedSerial = 0;
    auto notifiedIt = d_notifiedMasters.find(domainId);
    if (notifiedIt != d_notifiedMasters.end()) {
      notifiedSerial = notifiedIt->second;
    }
    domainInfo["notified_serial"] = notifiedSerial;

    if (notifiedSerial != zoneSOASerial) {
      allZones.push_back(domainInfo);
    }
  }
  nlohmann::json ret = {{"result", allZones}};
  sendResponse(request, response, ret);
}
} // namespace pdns_sysrepo::remote_backend