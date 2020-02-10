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

#include <spdlog/spdlog.h>
// #include <fmt/format.h>
#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::lookup(const Rest::Request& request, Http::ResponseWriter response) {
  logRequest(request);
  auto recordname = request.param(":recordname").as<std::string>();
  recordname = urlDecode(recordname);
  auto recordtype = request.param(":type").as<std::string>();

  string zoneName;
  try {
    zoneName = findBestZone(recordname);
  } catch (const std::out_of_range &e) {
    nlohmann::json::array_t err;
    err.push_back(e.what());
    sendResponse(request, response, nlohmann::json({{"result", nlohmann::json::array()}, {"log", err}}));
    return;
  }

  auto session = getSession();
  session->session_switch_ds(SR_DS_OPERATIONAL);
  auto tree = session->get_subtree("/pdns-server:zones");

  string rrsetLocation = "rrset";
  string zonetype = std::make_shared<libyang::Data_Node_Leaf_List>(tree->find_path(fmt::format("/pdns-server:zones/zones[name='{}']/zonetype", zoneName).c_str())->data().at(0))->value_str();
  if (zonetype =="slave") {
    rrsetLocation = "rrset-state";
  }

  string xpathBase = fmt::format("/pdns-server:zones/zones[name='{}']/{}[owner='{}']", zoneName, rrsetLocation, recordname);
  string xpathRecords = fmt::format("{}[type={}]", xpathBase, recordtype == "ANY" ? "*" : "'" + recordtype + "'");
  spdlog::trace("Attempting to find record node at {}", xpathRecords);
  auto nodeSet = tree->find_path(xpathRecords.c_str());

  nlohmann::json::array_t allRecords;

  // Each node here is one rrset, so we need to make N records from the N rdata's
  for (auto const &node : nodeSet->data()) {
    spdlog::trace("Found node in xpath={} node_path={} node_name={}", xpathRecords, node->path(), node->schema()->name());
    auto records = getRecordsFromRRSetNode(node, rrsetLocation);
    allRecords.insert(
      allRecords.end(),
      std::make_move_iterator(records.begin()),
      std::make_move_iterator(records.end())
    );
  }
  nlohmann::json ret = {{"result", allRecords}};
  sendResponse(request, response, ret);
}
} // namespace pdns_sysrepo::remote_backend
