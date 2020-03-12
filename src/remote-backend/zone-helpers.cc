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
#include "iputils/iputils.hh"

namespace pdns_sysrepo::remote_backend
{
nlohmann::json::array_t
RemoteBackend::getListofIPs(const sysrepo::S_Session& session, const libyang::S_Data_Node& node, const IpLeafRef& kind) {
  if (session == nullptr) {
    throw std::logic_error("session is nullptr");
  }
  string zoneElementSchemaPath = "/pdns-server:zones/pdns-server:zones/pdns-server:";
  string zoneElementSchemaName;
  string leafRefXPathTemplate = "/pdns-server:";
  switch (kind) {
  case IpLeafRef::master:
    zoneElementSchemaName = "masters";
    zoneElementSchemaPath = fmt::format("{}{}", zoneElementSchemaPath, zoneElementSchemaName);
    leafRefXPathTemplate += "master-endpoint[name='{}']";
    break;
  case IpLeafRef::also_notify:
    zoneElementSchemaName = "also-notify";
    zoneElementSchemaPath = fmt::format("{}{}", zoneElementSchemaPath, zoneElementSchemaName);
    leafRefXPathTemplate += "notify-endpoint[name='{}']";
    break;
  default:
    throw std::logic_error("getListOfIPs called with invalid IpLeafRef");
  }

  nlohmann::json::array_t ret;
  for (auto const itNode : node->tree_for()) {
    if (string(itNode->schema()->name()) != zoneElementSchemaName) {
      break;
    }
    auto nameLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(itNode);
    auto leafRefXPath = fmt::format(leafRefXPathTemplate, nameLeaf->value_str());
    auto leafRefAddressNode = session->get_subtree(leafRefXPath.c_str());
    for (auto const& addressNode : leafRefAddressNode->find_path(fmt::format("{}/{}", leafRefXPath, "/address").c_str())->data()) {
      std::string ip_address;
      uint16_t port = 53;
      for (auto const& node : addressNode->tree_dfs()) {
        std::string nodeName(node->schema()->name());
        if (nodeName == "ip-address") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          ip_address = leaf->value_str();
        }
        if (nodeName == "port") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          port = leaf->value()->uint16();
        }
      }
      iputils::ComboAddress address(ip_address, port);
      ret.push_back(address.toStringWithPort());
    }
  }
  return ret;
}

nlohmann::json::array_t
RemoteBackend::getListofNetmasks(const sysrepo::S_Session& session, const libyang::S_Data_Node& node, const NetmaskLeafRef& kind) {
  if (session == nullptr) {
    throw std::logic_error("session is nullptr");
  }
  string zoneElementSchemaPath = "/pdns-server:zones/pdns-server:zones/pdns-server:";
  string zoneElementSchemaName;
  string leafRefXPathTemplate = "/pdns-server:";
  switch (kind) {
  case NetmaskLeafRef::allow_axfr:
    zoneElementSchemaName = "allow-axfr";
    zoneElementSchemaPath = fmt::format("{}{}", zoneElementSchemaPath, zoneElementSchemaName);
    leafRefXPathTemplate += "axfr-access-control-list[name='{}']";
    break;
  default:
    throw std::logic_error("getListOfNetmasks called with invalid NetmaskLeafRef");
  }

  nlohmann::json::array_t ret;
  for (auto const itNode : node->tree_for()) {
    if (string(itNode->schema()->name()) != zoneElementSchemaName) {
      break;
    }
    auto nameLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(itNode);
    auto leafRefXPath = fmt::format(leafRefXPathTemplate, nameLeaf->value_str());
    auto leafRefAddressNode = session->get_subtree(leafRefXPath.c_str());
    for (auto const& netmaskNode : leafRefAddressNode->find_path(fmt::format("{}/{}", leafRefXPath, "/network/ip-prefix").c_str())->data()) {
      auto netmaskLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(netmaskNode);
      ret.push_back(netmaskLeaf->value_str());
    }
  }
  return ret;
}

nlohmann::json RemoteBackend::getDomainInfoFor(const string& domain) {
  auto session = getSession();

  auto bestZone = findBestZone(domain);
  if (bestZone != domain) {
    throw std::out_of_range(fmt::format("{} is not a domain", domain));
  }

  auto zoneNodeXPath = fmt::format("/pdns-server:zones/zones[name='{}']", domain);
  auto zoneNode = session->get_subtree(zoneNodeXPath.c_str());
  if (!zoneNode) {
    throw std::out_of_range(fmt::format("Unable to get zone node for path={}", zoneNodeXPath));
  }

  nlohmann::json ret;
  ret["zone"] = domain;
  ret["id"] = getDomainID(domain);

  bool allowAxfrDone = false;
  bool alsoNotifyDone = false;
  bool mastersDone = false;
  for (auto const& node : zoneNode->child()->tree_for()) {
    auto nodeName = string(node->schema()->name());
    if (nodeName == "zonetype") {
      auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
      ret["kind"] = leaf->value_str();
    }
    else if (!allowAxfrDone && nodeName == "allow-axfr") {
      ret["allow-axfr"] = getListofNetmasks(session, node, NetmaskLeafRef::allow_axfr);
      allowAxfrDone = true;
    }
    else if (!alsoNotifyDone && nodeName == "also-notify") {
      ret["also-notify"] = getListofIPs(session, node, IpLeafRef::also_notify);
      alsoNotifyDone = true;
    }
    else if (!mastersDone && nodeName == "masters") {
      ret["masters"] = getListofIPs(session, node, IpLeafRef::master);
      mastersDone = true;
    }
  }
  return ret;
}
} // namespace pdns_sysrepo::remote_backend