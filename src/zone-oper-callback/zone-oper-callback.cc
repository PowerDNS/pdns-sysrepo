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

#include <boost/optional.hpp>

#include "zone-oper-callback.hh"
#include "spdlog/spdlog.h"
#include "api/ZonesApi.h"

namespace pdns_sysrepo::zone_oper_callback
{

std::shared_ptr<ZoneCB> getZoneCB(std::shared_ptr<pdns_api::ApiClient>& apiClient) {
  auto cb = std::make_shared<ZoneCB>(ZoneCB(apiClient));
  return cb;
}

int ZoneCB::oper_get_items(sysrepo::S_Session session, const char* module_name,
  const char* path, const char* request_xpath,
  uint32_t request_id, libyang::S_Data_Node& parent, void* private_data) {
  spdlog::trace("oper_get_items called path={} request_xpath={}",
    path, (request_xpath != nullptr) ? request_xpath : "<none>");

  libyang::S_Context ctx = session->get_context();
  libyang::S_Module mod = ctx->get_module(module_name);
  parent.reset(new libyang::Data_Node(ctx, "/pdns-server:zones-state", nullptr, LYD_ANYDATA_CONSTSTRING, 0));

  pdns_api::ZonesApi zoneApiClient(d_apiClient);
  try {
    // We use a blocking call
    auto res = zoneApiClient.listZones("localhost", boost::none);
    auto zones = res.get();
    for (const auto& zone : zones) {
      libyang::S_Data_Node zoneNode(new libyang::Data_Node(parent, mod, "zones"));
      libyang::S_Data_Node classNode(new libyang::Data_Node(zoneNode, mod, "class", "IN"));
      libyang::S_Data_Node nameNode(new libyang::Data_Node(zoneNode, mod, "name", zone->getName().c_str()));
      libyang::S_Data_Node serialNode(new libyang::Data_Node(zoneNode, mod, "serial", std::to_string(zone->getSerial()).c_str()));

      // Lowercase zonekind
      std::string zoneKind = zone->getKind();
      std::transform(zoneKind.begin(), zoneKind.end(), zoneKind.begin(), [](unsigned char c) { return std::tolower(c); });
      libyang::S_Data_Node zonetypeNode(new libyang::Data_Node(zoneNode, mod, "zonetype", zoneKind.c_str()));

      if (zoneKind == "slave") {
        for (auto const& master : zone->getMasters()) {
          libyang::S_Data_Node(new libyang::Data_Node(zoneNode, mod, "masters", master.c_str()));
        }
      }
    }
  }
  catch (const web::uri_exception& e) {
    spdlog::warn("Unable to retrieve zones from from server: {}", e.what());
  }
  catch (const web::http::http_exception& e) {
    spdlog::warn("Unable to retrieve zones from from server: {}", e.what());
  }
  catch (const std::exception& e) {
    spdlog::warn("Could not create zonestatus: {}", e.what());
  }

  return SR_ERR_OK;
}
} // namespace pdns_sysrepo::zone_oper_callback