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

#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>

#include "pdns-config-callback.hh"
#include "util/util.hh"
#include "sr_wrapper/session.hh"

namespace pdns_api = org::openapitools::client::api;
namespace pdns_api_model = org::openapitools::client::model;
namespace fs = boost::filesystem;

namespace pdns_sysrepo::pdns_config
{
std::shared_ptr<PdnsConfigCB> getServerConfigCB(std::shared_ptr<pdns_sysrepo::config::Config> &config, std::shared_ptr<pdns_api::ApiClient> &apiClient) {
  auto cb = std::make_shared<PdnsConfigCB>(PdnsConfigCB(
    config,
    apiClient));
  return cb;
}

std::string PdnsConfigCB::tmpFile(const uint32_t request_id) {
  return d_config->getPdnsConfigFilename() + "-tmp-" + std::to_string(request_id);
}

int PdnsConfigCB::module_change(sysrepo::S_Session session, const char* module_name,
  const char* xpath, sr_event_t event,
  uint32_t request_id, void* private_data) {
  spdlog::trace("Had callback. module_name={} xpath={} event={} request_id={}",
    (module_name == nullptr) ? "" : module_name,
    (xpath == nullptr) ? "" : xpath,
    util::srEvent2String(event), request_id);

  if (event == SR_EV_ENABLED) {
    try {
      changeConfigUpdate(session, request_id);
    } catch(const std::exception &e) {
      spdlog::warn("Unable to create temporary config: {}", e.what());
      return SR_ERR_OPERATION_FAILED;
    }
  }

  if (event == SR_EV_CHANGE) {
    try {
      changeConfigUpdate(session, request_id);
    } catch(const std::exception &e) {
      spdlog::warn("Unable to create temporary config: {}", e.what());
      return SR_ERR_OPERATION_FAILED;
    }

    if (d_apiClient != nullptr) {
      try {
        changeZoneAddAndDelete(session);
      }
      catch (const std::exception& e) {
        spdlog::warn("Zone changes not possible: {}", e.what());
        return SR_ERR_OPERATION_FAILED;
      }

      try {
        changeZoneModify(session);
      }
      catch (const std::exception& e) {
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
      catch (const std::exception& e) {
        spdlog::warn("Unable to move {} to {}: {}", fpath, d_config->getPdnsConfigFilename(), e.what());
        fs::remove(fpath);
        return SR_ERR_OPERATION_FAILED;
      }

      try {
        // TODO Are we responsible for this, or should we let the network service controller decide?
        session->session_switch_ds(SR_DS_STARTUP);
        session->copy_config(SR_DS_RUNNING, module_name);
        session->session_switch_ds(SR_DS_RUNNING);
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
            auto sess = std::static_pointer_cast<sr::Session>(session);
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
      catch (const std::exception& e) {
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


}