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

  // TODO make fed records do this?
  struct rrset {
    std::vector<string> rdata;
    uint32_t ttl;
  };
  typedef std::map<string, std::map<string, rrset > > rrsets_t;

  void RemoteBackend::commitTransaction(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
    logRequest(request);
    uint32_t txId = request.param(":txid").as<uint32_t>();
    auto it = d_transactions.find(txId);
    if (it == d_transactions.end()) {
      sendResponse(request, response, nlohmann::json({{"result", false}, {"log", nlohmann::json::array_t({"No such transaction"})}}));
      return;
    }

    auto records = it->second->getFedRecords();
    rrsets_t rrsets;

    auto session = getSession();
    session->session_switch_ds(SR_DS_OPERATIONAL);

    string baseXPath = fmt::format("/pdns-server:zones/zones[name='{}']/rrset-state", it->second->getDomainName());
    try {
      for (auto const& i : records) {
        auto rrsetXPath = fmt::format("{}[owner='{}'][type='{}']/", baseXPath, i.qname, i.qtype);
        session->set_item_str(fmt::format("{}ttl", rrsetXPath).c_str(), std::to_string(i.ttl).c_str());
        auto rdataXPath = fmt::format("{}rdata/", rrsetXPath);
        if (i.qtype == "SOA") {
          std::vector<std::string> parts;
          boost::split(parts, i.content, boost::is_any_of(" "));
          session->set_item_str(fmt::format("{}SOA/mname", rdataXPath).c_str(), parts.at(0).c_str());
          session->set_item_str(fmt::format("{}SOA/rname", rdataXPath).c_str(), parts.at(1).c_str());
          session->set_item_str(fmt::format("{}SOA/serial", rdataXPath).c_str(), parts.at(2).c_str());
          session->set_item_str(fmt::format("{}SOA/refresh", rdataXPath).c_str(), parts.at(3).c_str());
          session->set_item_str(fmt::format("{}SOA/retry", rdataXPath).c_str(), parts.at(4).c_str());
          session->set_item_str(fmt::format("{}SOA/expire", rdataXPath).c_str(), parts.at(5).c_str());
          session->set_item_str(fmt::format("{}SOA/minimum", rdataXPath).c_str(), parts.at(6).c_str());
        }
        else if (i.qtype == "A" || i.qtype == "AAAA") {
          session->set_item_str(fmt::format("{}{}/address", rdataXPath, i.qtype).c_str(), i.content.c_str());
        }
        else if (i.qtype == "NS") {
          session->set_item_str(fmt::format("{}{}/nsdname", rdataXPath, i.qtype).c_str(), i.content.c_str());
        }
        else {
          throw std::logic_error(fmt::format("Unimplemented record type: {}", i.qtype));
        }
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
      session->apply_changes();
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
    FedRecord f;
    f.qname = qname;
    f.qtype = qtype;
    f.content = content;
    f.ttl = ttl;
    const std::lock_guard<std::mutex> lock(d_lock);
    d_records.push_back(f);
  }
}