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

namespace pdns_sysrepo::remote_backend
{
  void RemoteBackend::startTransaction(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
      logRequest(request);
      uint32_t zoneId = request.param(":id").as<uint32_t>();
      uint32_t txId = request.param(":txid").as<uint32_t>();
      string domainName = request.param(":zone").as<string>();
      domainName = urlDecode(domainName);

      try {
        auto zoneFromId = getDomainFromId(zoneId);
        if (zoneFromId != domainName) {
            throw std::out_of_range(fmt::format("Domain for id {} is not {}, but {}", zoneId, domainName, zoneFromId));
        }
      } catch(const std::out_of_range &e) {
        sendResponse(request, response, nlohmann::json({{"result", false}, {"log", nlohmann::json::array_t({e.what()})}}));
        return;
      }

      auto it = d_transactions.find(txId);
      if (it != d_transactions.end()) {
        sendResponse(request, response, nlohmann::json({{"result", false}, {"log", nlohmann::json::array_t({"Transaction already in progress"})}}));
        return;
      }

      d_transactions.insert(std::make_pair(txId, std::make_shared<Transaction>(txId, zoneId, domainName)));
      sendResponse(request, response, nlohmann::json({{"result", true}}));
  }

  void RemoteBackend::abortTransaction(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
    logRequest(request);
    uint32_t txId = request.param(":txid").as<uint32_t>();
    auto it = d_transactions.find(txId);
    if (it != d_transactions.end()) {
      d_transactions.erase(it);
    }
    sendResponse(request, response, nlohmann::json({{"result", true}}));
  }

  void RemoteBackend::commitTransaction(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
    logRequest(request);
    uint32_t txId = request.param(":txid").as<uint32_t>();
    auto it = d_transactions.find(txId);
    if (it == d_transactions.end()) {
      sendResponse(request, response, nlohmann::json({{"result", false}, {"log", nlohmann::json::array_t({"Transaction already in progress"})}}));
      return;
    }
    // TODO Pushing to sysrepo here
    d_transactions.erase(it);
    sendResponse(request, response, nlohmann::json({{"result", true}}));
  }
}