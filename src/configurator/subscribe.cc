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

#include "subscribe.hh"
#include "configurator.hh"

using namespace std;

namespace pdns_conf
{
sysrepo::S_Callback getServerConfigCB(const string& fpath) {
  sysrepo::S_Callback cb(new ServerConfigCB(
    {{"fpath", fpath}}));
  return cb;
}

int ServerConfigCB::module_change(sysrepo::S_Session session, const char* module_name,
  const char* xpath, sr_event_t event,
  uint32_t request_id, void* private_data) {
  spdlog::debug("Had callback module_name={} xpath={} event={}",
    (module_name == nullptr) ? "" : module_name,
    (xpath == nullptr) ? "" : xpath,
    event);

  if (event == SR_EV_CHANGE) {
    // Request to change things, write a tempfile
    // todo store tempfile name in private_data
    string fpath = privData["fpath"];
    spdlog::debug("Writing new config to {}", fpath);
    writeConfig(fpath, vector<sysrepo::S_Val>());
    spdlog::debug("config written");
  }

  if (event == SR_EV_DONE) {
    // TODO copy the file and restart powerdns
    try {
      // Copy the running to the startup config
      // TODO Are e responsible for this?
      session->copy_config(SR_DS_RUNNING, SR_DS_STARTUP, module_name);
    }
    catch (const sysrepo::sysrepo_exception& se) {
      spdlog::warn("Could not copy running config to startup config");
      return SR_ERR_OPERATION_FAILED;
    }
  }

  if (event == SR_EV_ABORT) {
    // TODO Rollback changes
  }
  return SR_ERR_OK;
}
} // namespace pdns_conf