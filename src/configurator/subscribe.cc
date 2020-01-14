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

#include <boost/filesystem.hpp>

#include "api/ZonesApi.h"
#include "subscribe.hh"
#include "configurator.hh"
#include "util.hh"
#include "sr_wrapper/session.hh"
#include "config/config.hh"

namespace fs = boost::filesystem;

namespace pdns_conf
{
std::shared_ptr<ServerConfigCB> getServerConfigCB(shared_ptr<pdns_sysrepo::config::Config> &config, shared_ptr<pdns_api::ApiClient> &apiClient) {
  auto cb = make_shared<ServerConfigCB>(ServerConfigCB(
    config,
    apiClient));
  return cb;
}

std::shared_ptr<ZoneCB> getZoneCB(shared_ptr<pdns_api::ApiClient> &apiClient) {
  auto cb = make_shared<ZoneCB>(ZoneCB(apiClient));
  return cb;
}

string ServerConfigCB::tmpFile(const uint32_t request_id) {
  return d_config->getPdnsConfigFilename() + "-tmp-" + to_string(request_id);
}

int ServerConfigCB::module_change(sysrepo::S_Session session, const char* module_name,
  const char* xpath, sr_event_t event,
  uint32_t request_id, void* private_data) {
  spdlog::trace("Had callback. module_name={} xpath={} event={} request_id={}",
    (module_name == nullptr) ? "" : module_name,
    (xpath == nullptr) ? "" : xpath,
    util::srEvent2String(event), request_id);

  if (event == SR_EV_ENABLED) {
    try {
      changeConfigUpdate(session, request_id);
    } catch(const exception &e) {
      spdlog::warn("Unable to create temporary config: {}", e.what());
      return SR_ERR_OPERATION_FAILED;
    }
  }

  if (event == SR_EV_CHANGE) {
    try {
      changeConfigUpdate(session, request_id);
    } catch(const exception &e) {
      spdlog::warn("Unable to create temporary config: {}", e.what());
      return SR_ERR_OPERATION_FAILED;
    }

    if (d_apiClient != nullptr) {
      try {
        changeZoneAddAndDelete(session);
      }
      catch (const exception& e) {
        spdlog::warn("Zone changes not possible: {}", e.what());
        return SR_ERR_OPERATION_FAILED;
      }

      try {
        changeZoneModify(session);
      }
      catch (const exception& e) {
        spdlog::warn("Zone modifications not possible: {}", e.what());
        return SR_ERR_OPERATION_FAILED;
      }
    }
  }

  if (event == SR_EV_DONE) {
    if (pdnsConfigChanged) {
      pdnsConfigChanged = false;
      auto fpath = tmpFile(request_id);

      try {
        spdlog::debug("Moving {} to {}", fpath, d_config->getPdnsConfigFilename());
        fs::rename(fpath, d_config->getPdnsConfigFilename());
      }
      catch (const exception& e) {
        spdlog::warn("Unable to move {} to {}: {}", fpath, d_config->getPdnsConfigFilename(), e.what());
        fs::remove(fpath);
        return SR_ERR_OPERATION_FAILED;
      }

      try {
        // TODO Are we responsible for this, or should we let the network service controller decide?
        session->copy_config(SR_DS_RUNNING, SR_DS_STARTUP, module_name);
      }
      catch (const sysrepo::sysrepo_exception& se) {
        spdlog::warn("Could not copy running config to startup config");
        return SR_ERR_OPERATION_FAILED;
      }

      if (!d_config->getServiceName().empty()) {
        restartService(d_config->getServiceName());
      }

      if (d_apiClient != nullptr) {
        if (apiConfigChanged) {
          try {
            auto sess = static_pointer_cast<sr::Session>(session);
            configureApi(sess->getConfigTree());
            apiConfigChanged = false;
          }
          catch (const std::exception& e) {
            spdlog::warn("Could not initiate API Client: {}", e.what());
            apiConfigChanged = false;
            return SR_ERR_OPERATION_FAILED;
          }
        }
      }

      try {
        doneZoneAddAndDelete();
        doneZoneModify();
      }
      catch (const std::exception& e) {
        spdlog::warn("Zone manipulation failed: {}", e.what());
        return SR_ERR_OPERATION_FAILED;
      }
    }
  }

  if (event == SR_EV_ABORT) {
    if (pdnsConfigChanged) {
      pdnsConfigChanged = false;
      auto fpath = tmpFile(request_id);
      try {
        fs::remove(fpath);
      }
      catch (const exception& e) {
        spdlog::warn("Unable to remove temporary file fpath={}: {}", fpath, e.what());
      }
    }
    if (apiConfigChanged) {
      apiConfigChanged = false;
    }
    zonesCreated.clear();
    zonesRemoved.clear();
    zonesModified.clear();
  }
  return SR_ERR_OK;
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
    for (const auto &zone : zones) {
      libyang::S_Data_Node zoneNode(new libyang::Data_Node(parent, mod, "zones"));
      libyang::S_Data_Node classNode(new libyang::Data_Node(zoneNode, mod, "class", "IN"));
      libyang::S_Data_Node nameNode(new libyang::Data_Node(zoneNode, mod, "name", zone->getName().c_str()));
      libyang::S_Data_Node serialNode(new libyang::Data_Node(zoneNode, mod, "serial", to_string(zone->getSerial()).c_str()));

      // Lowercase zonekind
      string zoneKind = zone->getKind();
      std::transform(zoneKind.begin(), zoneKind.end(), zoneKind.begin(), [](unsigned char c){ return std::tolower(c); });
      libyang::S_Data_Node zonetypeNode(new libyang::Data_Node(zoneNode, mod, "zonetype", zoneKind.c_str()));

      if (zoneKind == "slave") {
        for (auto const &master: zone->getMasters()) {
          libyang::S_Data_Node(new libyang::Data_Node(zoneNode, mod, "masters", master.c_str()));
        }
      }

    }
  } catch (const web::uri_exception &e) {
    spdlog::warn("Unable to retrieve zones from from server: {}", e.what());
  } catch (const web::http::http_exception &e) {
    spdlog::warn("Unable to retrieve zones from from server: {}", e.what());
  } catch (const std::exception &e) {
    spdlog::warn("Could not create zonestatus: {}", e.what());
  }

  return SR_ERR_OK;
}
} // namespace pdns_conf