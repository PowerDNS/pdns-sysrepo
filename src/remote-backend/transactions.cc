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
#include <libyang/Tree_Data.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

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
      sendResponse(request, response, nlohmann::json({{"result", false}, {"log", nlohmann::json::array_t({"No such transaction"})}}));
      return;
    }

    auto records = it->second->getFedRecords();
    auto session = getSession();
    session->session_switch_ds(SR_DS_OPERATIONAL);

    string zoneXPath = fmt::format("/pdns-server:zones/zones[name='{}']", it->second->getDomainName());
    nlohmann::json tree;
    try {
      auto full_tree = session->get_subtree(zoneXPath.c_str());
      tree["pdns-server:zones"] = nlohmann::json::parse(full_tree->print_mem(LYD_JSON, 0));
      tree["pdns-server:zones"]["pdns-server:zones"][0].erase("rrset-state");

      for (auto const& i : records) {
        nlohmann::json jsonRecord;
        jsonRecord["owner"] = std::get<0>(i.first);
        jsonRecord["ttl"] = std::get<2>(i.first);

        auto recordType = std::get<1>(i.first);
        jsonRecord["type"] = recordType;

        for (auto const& content : i.second) {
          if (recordType == "SOA") {
            std::vector<std::string> parts;
            boost::split(parts, content, boost::is_any_of(" "));

            jsonRecord["rdata"]["SOA"]["mname"] = parts.at(0);
            jsonRecord["rdata"]["SOA"]["rname"] = parts.at(1);
            jsonRecord["rdata"]["SOA"]["serial"] = std::atoi(parts.at(2).c_str());
            jsonRecord["rdata"]["SOA"]["refresh"] = std::atoi(parts.at(3).c_str());
            jsonRecord["rdata"]["SOA"]["retry"] = std::atoi(parts.at(4).c_str());
            jsonRecord["rdata"]["SOA"]["expire"] = std::atoi(parts.at(5).c_str());
            jsonRecord["rdata"]["SOA"]["minimum"] = std::atoi(parts.at(6).c_str());
          }
          else if (recordType == "A" || recordType == "AAAA") {
            if (!jsonRecord["rdata"][recordType]["address"].is_array()) {
              jsonRecord["rdata"][recordType]["address"] = nlohmann::json::array();
            }
            jsonRecord["rdata"][recordType]["address"].push_back(content);
          }
          else if (recordType == "PTR") {
            if (!jsonRecord["rdata"][recordType]["ptrdname"].is_array()) {
              jsonRecord["rdata"][recordType]["ptrdname"] = nlohmann::json::array();
            }
            jsonRecord["rdata"][recordType]["ptrdname"].push_back(content);
          }
          else if (recordType == "NS") {
            if (!jsonRecord["rdata"][recordType]["nsdname"].is_array()) {
              jsonRecord["rdata"][recordType]["nsdname"] = nlohmann::json::array();
            }
            jsonRecord["rdata"][recordType]["nsdname"].push_back(content);
          }
          else if (recordType == "TXT") {
            if (!jsonRecord["rdata"][recordType]["txt-data"].is_array()) {
              jsonRecord["rdata"][recordType]["txt-data"] = nlohmann::json::array();
            }
            jsonRecord["rdata"][recordType]["txt-data"].push_back(content);
          }
          else if (recordType == "CNAME") {
            jsonRecord["rdata"][recordType]["cname"]= content;
          }
          else if (recordType == "DNAME") {
            jsonRecord["rdata"][recordType]["target"]= content;
          }
          else if (recordType == "MX") {
            if (!jsonRecord["rdata"][recordType]["mail-exchanger"].is_array()) {
              jsonRecord["rdata"][recordType]["mail-exchanger"] = nlohmann::json::array();
            }
            std::vector<std::string> parts;
            boost::split(parts, content, boost::is_any_of(" "));
            nlohmann::json mx;
            mx["preference"] = std::atoi(parts.at(0).c_str());
            mx["exchange"] = parts.at(1);
            jsonRecord["rdata"][recordType]["mail-exchanger"].push_back(mx);
          }
          else {
            throw std::logic_error(fmt::format("Unimplemented record type: {}", recordType));
          }
        }
        tree["pdns-server:zones"]["pdns-server:zones"][0]["rrset-state"].push_back(jsonRecord);
      }
    } catch (const std::exception& e) {
      spdlog::warn("Exception while setting items: {}", e.what());
      auto errors = getErrorsFromSession(session);
      for (auto const &error : errors) {
        spdlog::warn("error={} xpath={}", error.first, error.second);
      }
      sendError(request, response, e.what());
      return;
    }
    try {
      // Prepare the changes
      auto newData = tree.dump();
      auto ctx = session->get_context();
      auto newNode = ctx->parse_data_mem(newData.c_str(), LYD_JSON, LYD_OPT_GET); // Why LYD_OPT_GET? see https://github.com/sysrepo/sysrepo/issues/1804#issuecomment-583309999

      // Yes! there is a race condition here as we delete the whole zone from the operational
      // datastore and _then_ add it with the new data (see https://github.com/sysrepo/sysrepo/issues/1809).
      // During some testing, the zone is gone from the datastore between 0.005 and 0.008 seconds
      spdlog::debug("RemoteBackend::commitTransaction - Removing zone for update xpath='{}'", zoneXPath);
      session->delete_item(zoneXPath.c_str(), 0);
      session->apply_changes();

      spdlog::debug("RemoteBackend::commitTransaction - Updating zone json='{}'", newData);
      session->edit_batch(newNode, "merge");
    } catch (const std::exception& e) {
      spdlog::warn("Exception in edit_batch: {}", e.what());
      auto errors = getErrorsFromSession(session);
      for (auto const &error : errors) {
        spdlog::warn("error={} xpath={}", error.first, error.second);
      }
      sendError(request, response, e.what());
      return;
    }
    try {
      session->validate();
    } catch (const std::exception& e) {
      spdlog::warn("Exception in validate: {}", e.what());
      auto errors = getErrorsFromSession(session);
      for (auto const &error : errors) {
        spdlog::warn("error={} xpath={}", error.first, error.second);
      }
      sendError(request, response, e.what());
      return;
    }
    try {
      session->apply_changes();
      spdlog::debug("RemoteBackend::commitTransaction - Finished updating zone='{}'", it->second->getDomainName());
    } catch (const std::exception& e) {
      spdlog::warn("Exception in apply_changes: {}", e.what());
      auto errors = getErrorsFromSession(session);
      for (auto const &error : errors) {
        spdlog::warn("error={} xpath={}", error.first, error.second);
      }
      sendError(request, response, e.what());
      return;
    }
    d_transactions.erase(it);
    sendResponse(request, response, nlohmann::json({{"result", true}}));
  }

  void RemoteBackend::Transaction::feedRecord(const string& qname, const string& qtype, const string& content, const uint32_t ttl) {
    FedRRSet f = std::make_tuple(qname, qtype, ttl);
    const std::lock_guard<std::mutex> lock(d_lock);
    d_records[f].push_back(content);
  }
}