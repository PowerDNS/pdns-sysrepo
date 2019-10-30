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

#include <boost/filesystem.hpp>

#include "subscribe.hh"
#include "configurator.hh"
#include "util.hh"
#include "sr_wrapper/session.hh"

using namespace std;
namespace fs = boost::filesystem;

namespace pdns_conf
{
sysrepo::S_Callback getServerConfigCB(const string& fpath) {
  sysrepo::S_Callback cb(new ServerConfigCB(
    {{"fpath", fpath}}));
  return cb;
}

string ServerConfigCB::tmpFile(const uint32_t request_id) {
  return privData["fpath"] + "-tmp-" + to_string(request_id);
}

int ServerConfigCB::module_change(sysrepo::S_Session session, const char* module_name,
  const char* xpath, sr_event_t event,
  uint32_t request_id, void* private_data) {
  spdlog::trace("Had callback. module_name={} xpath={} event={} request_id={}",
    (module_name == nullptr) ? "" : module_name,
    (xpath == nullptr) ? "<none>" : xpath,
    srEvent2String(event), request_id);

  if (event == SR_EV_CHANGE) {
    auto fpath = tmpFile(request_id);
    auto sess = static_pointer_cast<sr::Session>(session);

    // The session already has the new datastore values
    PdnsServerConfig c(sess->getConfigTree());

    c.writeToFile(fpath);
  }

  if (event == SR_EV_DONE) {
    auto fpath = tmpFile(request_id);

    try {
      fs::rename(fpath, privData["fpath"]);
    } catch (const exception &e) {
      spdlog::warn("Unable to move {} to {}: {}", fpath, privData["fpath"], e.what());
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
    // TODO signal systemd for a pdns restart
  }

  if (event == SR_EV_ABORT) {
    auto fpath = tmpFile(request_id);
    try {
      fs::remove(fpath);
    } catch (const exception &e) {
      spdlog::warn("Unable to remove temporary file fpath={}: {}", fpath, e.what());
    }
  }
  return SR_ERR_OK;
}
} // namespace pdns_conf