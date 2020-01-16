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

namespace pdns_sysrepo::remote_backend {
void RemoteBackend::list(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
  logRequest(request);
  auto zoneName = request.param(":zone").as<std::string>();
  zoneName = urlDecode(zoneName);

  try {
    auto foundZone = findBestZone(zoneName);
    if (foundZone != zoneName) {
      throw std::runtime_error(fmt::format("Zone '{}' not found (best match is '{})", zoneName, foundZone));
    }
  } catch(const std::exception &e) {
    nlohmann::json::array_t log;
    log.push_back(e.what());
    sendResponse(request, response, nlohmann::json({{"result", nlohmann::json::array_t()}, {"log", log}}));
    return;
  }

  nlohmann::json::array_t allRecords;

  auto session = getSession();
  auto zoneXPath = fmt::format("/pdns-server:zones/zones[name='{}']", zoneName);

  spdlog::trace("remote_backend/list - Grabbing zone xpath={}", zoneXPath);
  auto tree = session->get_subtree(zoneXPath.c_str());
  spdlog::trace("remote_backend/list - tree path={}", tree->path());

  for (auto const& rrsetNode : tree->child()->tree_for()) {
    if (string(rrsetNode->schema()->name()) != "rrset") {
      continue;
    }
    auto records = getRecordsFromRRSetNode(rrsetNode);
    allRecords.insert(
      allRecords.end(),
      std::make_move_iterator(records.begin()),
      std::make_move_iterator(records.end())
    );
  }
  nlohmann::json ret = {{"result", allRecords}};

  sendResponse(request, response, ret);
}
}