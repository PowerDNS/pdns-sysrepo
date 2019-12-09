/*
 * Copyright 2019 Pieter Lexis <pieter.lexis@powerdns.com>
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
#include "../subscribe.hh"
#include "ZonesApi.h"

namespace pdns_conf
{
string ServerConfigCB::getZoneId(const string& zone) {
  pdns_api::ZonesApi zonesApiClient(d_apiClient);

  auto zones = zonesApiClient.listZones("localhost", zone).get();
  if (zones.size() != 1) {
    throw runtime_error(fmt::format("API returned the wrong number of zones ({}) for '{}', expected 1", zones.size(), zone));
  }
  return zones.at(0)->getId();
}

void ServerConfigCB::configureApi(const libyang::S_Data_Node &node) {
  spdlog::trace("Reconfiguring API Client");
  auto apiConfig = make_shared<pdns_api::ApiConfiguration>();

  try {
    auto apiKeyNode = node->find_path("/pdns-server:pdns-server/webserver/api-key")->data().at(0);
    apiConfig->setApiKey("X-API-Key", make_shared<libyang::Data_Node_Leaf_List>(apiKeyNode)->value_str());
    auto addressNode = node->find_path("/pdns-server:pdns-server/webserver/address")->data().at(0);
    auto portNode = node->find_path("/pdns-server:pdns-server/webserver/port")->data().at(0);
    ComboAddress ws(string(make_shared<libyang::Data_Node_Leaf_List>(addressNode)->value_str()), make_shared<libyang::Data_Node_Leaf_List>(portNode)->value()->uint16());
    apiConfig->setBaseUrl("http://" + ws.toStringWithPort() + "/api/v1");
    spdlog::debug("API Client config set to: API-Key={}, URL={}", apiConfig->getApiKey("X-API-Key"), apiConfig->getBaseUrl());
  }
  catch (const out_of_range &e) {
    // ignore, We implicitly disable the API with the "wrong" config
    spdlog::debug("API is disabled in config, client will reflect this");
  }

  apiConfig->setUserAgent("pdns-sysrepo/" + string(VERSION));
  d_apiClient->setConfiguration(apiConfig);
  spdlog::trace("API Client reconfigured");
}
} // namespace pdns_conf