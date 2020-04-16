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

    auto records = it->second->getRRSets();
    auto session = getSession();
    session->session_switch_ds(SR_DS_OPERATIONAL);

    string zoneXPath = fmt::format("/pdns-server:zones/zones[name='{}']", it->second->getDomainName());
    string rrsetBaseXPath = fmt::format("{}/rrset-state", zoneXPath);
    try {
      spdlog::debug("RemoteBackend::commitTransaction - removing rrsets for zone={} xpath={}", it->second->getDomainName(), rrsetBaseXPath);
      session->delete_item(fmt::format("{}", rrsetBaseXPath).c_str());
      spdlog::trace("RemoteBackend::commitTransaction - removed rrsets for zone={} xpath={}", it->second->getDomainName(), rrsetBaseXPath);

      for (auto const& i : records) {
        auto owner = std::get<0>(i.first);
        auto recordType = std::get<1>(i.first);
        string rrsetXPath = fmt::format("{}[owner='{}'][type='{}']", rrsetBaseXPath, owner, recordType);
        session->set_item_str(fmt::format("{}/ttl", rrsetXPath).c_str(), std::to_string(std::get<2>(i.first)).c_str());
        for (auto const& content : i.second) {
          auto rdataXPath = fmt::format("{}/rdata", rrsetXPath);
          spdlog::trace("RemoteBackend::commitTransaction - setting content='{}' recordtype={} recordname={}", content, recordType, owner);
          if (recordType == "SOA") {
            std::vector<std::string> parts;
            boost::split(parts, content, boost::is_any_of(" "));
            session->set_item_str(fmt::format("{}/SOA/mname", rdataXPath).c_str(), parts.at(0).c_str());
            session->set_item_str(fmt::format("{}/SOA/rname", rdataXPath).c_str(), parts.at(1).c_str());
            session->set_item_str(fmt::format("{}/SOA/serial", rdataXPath).c_str(), parts.at(2).c_str());
            session->set_item_str(fmt::format("{}/SOA/refresh", rdataXPath).c_str(), parts.at(3).c_str());
            session->set_item_str(fmt::format("{}/SOA/retry", rdataXPath).c_str(), parts.at(4).c_str());
            session->set_item_str(fmt::format("{}/SOA/expire", rdataXPath).c_str(), parts.at(5).c_str());
            session->set_item_str(fmt::format("{}/SOA/minimum", rdataXPath).c_str(), parts.at(6).c_str());
          }
          else if (recordType == "A" || recordType == "AAAA") {
            session->set_item_str(fmt::format("{}/{}/address", rdataXPath, recordType).c_str(), content.c_str());
          }
          else if (recordType == "PTR") {
            session->set_item_str(fmt::format("{}/{}/ptrdname", rdataXPath, recordType).c_str(), content.c_str());
          }
          else if (recordType == "NS") {
            session->set_item_str(fmt::format("{}/{}/nsdname", rdataXPath, recordType).c_str(), content.c_str());
          }
          else if (recordType == "TXT") {
            session->set_item_str(fmt::format("{}/{}/txt-data", rdataXPath, recordType).c_str(), content.c_str());
          }
          else if (recordType == "CNAME") {
            session->set_item_str(fmt::format("{}/{}/cname", rdataXPath, recordType).c_str(), content.c_str());
          }
          else if (recordType == "DNAME") {
            session->set_item_str(fmt::format("{}/{}/target", rdataXPath, recordType).c_str(), content.c_str());
          }
          else if (recordType == "MX") {
            std::vector<std::string> parts;
            boost::split(parts, content, boost::is_any_of(" "));
            session->set_item_str(fmt::format("{}/{}/mail-exchanger[preference='{}'][exchange='{}']", rdataXPath, recordType, parts.at(0), parts.at(1)).c_str(), nullptr);
          } else if (recordType == "SRV") {
            std::vector<std::string> parts;
            boost::split(parts, content, boost::is_any_of(" "));
            session->set_item_str(fmt::format("{}/{}/service[priority='{}'][weight='{}'][port='{}'][target='{}']", rdataXPath, recordType, parts.at(0), parts.at(1), parts.at(2), parts.at(3)).c_str(), nullptr);
          } else {
            throw std::logic_error(fmt::format("Unimplemented record type: {}", recordType));
          }
        }
      }
    } catch (const std::exception& e) {
      spdlog::warn("RemoteBackend::commitTransaction - Could not set an item, aborting. error={}", e.what());
      logSessionErrors(session, spdlog::level::warn);
      sendError(request, response, e.what());
      return;
    }
    try {
      spdlog::trace("RemoteBackend::commitTransaction - Validating changes");
      session->validate();
      spdlog::trace("RemoteBackend::commitTransaction - Validated changes");
    } catch (const std::exception& e) {
      spdlog::warn("RemoteBackend::commitTransaction - Changes did not validate. error={}", e.what());
      logSessionErrors(session, spdlog::level::warn);
      sendError(request, response, e.what());
      return;
    }
    try {
      spdlog::trace("RemoteBackend::commitTransaction - Applying changes");
      session->apply_changes();
      spdlog::debug("RemoteBackend::commitTransaction - Finished updating zone='{}'", it->second->getDomainName());
    } catch (const std::exception& e) {
      spdlog::warn("RemoteBackend::commitTransaction - Unable to apply changes. error={}", e.what());
      logSessionErrors(session, spdlog::level::warn);
      sendError(request, response, e.what());
      return;
    }
    d_transactions.erase(it);
    sendResponse(request, response, nlohmann::json({{"result", true}}));
  }

  void RemoteBackend::Transaction::feedRecord(const string& qname, const string& qtype, const string& content, const uint32_t ttl) {
    RRSetKey f = std::make_tuple(qname, qtype, ttl);
    const std::lock_guard<std::mutex> lock(d_lock);
    d_records[f].push_back(content);
  }
}