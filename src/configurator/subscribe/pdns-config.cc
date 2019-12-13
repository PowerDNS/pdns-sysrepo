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

namespace pdns_conf
{
void ServerConfigCB::changeConfigUpdate(sysrepo::S_Session& session, const uint32_t request_id) {
  auto iter = session->get_changes_iter("/pdns-server:pdns-server//*");
  auto change = session->get_change_tree_next(iter);

  if (change != nullptr) {
    pdnsConfigChanged = true;
    auto fpath = tmpFile(request_id);

    auto sess = static_pointer_cast<sr::Session>(session);

    // The session already has the new datastore values
    PdnsServerConfig c(sess->getConfigTree(), session);
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