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
#include "pdns-config-callback.hh"
#include "sr_wrapper/session.hh"

namespace pdns_sysrepo::pdns_config
{
void PdnsConfigCB::changeConfigUpdate(sysrepo::S_Session& session, const uint32_t request_id) {
  auto iter = session->get_changes_iter("/pdns-server:pdns-server//*");
  auto change = session->get_change_tree_next(iter);

  if (change != nullptr) {
    if (!d_config->getDoServiceRestart() && !isFromEnabled) {
      session->set_error("Unable to process configuration changes, service may not restart", nullptr);
      throw std::runtime_error("Service restarts are disabled");
    }
    pdnsConfigChanged = true;
    auto fpath = tmpFile(request_id);

    auto sess = std::static_pointer_cast<sr::Session>(session);

    // The session already has the new datastore values
    PdnsServerConfig c(sess->getConfigTree(), session, (d_apiClient == nullptr));
    c.writeToFile(fpath);
  }

  iter = session->get_changes_iter("/pdns-server:pdns-server/webserver/*");
  change = session->get_change_tree_next(iter);
  while (change != nullptr) {
    string name = change->node()->schema()->name();
    if (name == "address" || name == "port" || name == "api-key") {
      apiConfigChanged = true;
      break;
    }
    change = session->get_change_tree_next(iter);
  }
}
} // namespace pdns_conf