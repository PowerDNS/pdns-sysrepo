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

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::feedRecord(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
  logRequest(request);
  uint32_t txId = request.param(":txid").as<uint32_t>();
  auto it = d_transactions.find(txId);
  if (it == d_transactions.end()) {
    sendResponse(request, response, nlohmann::json({{"result", false}, {"log", nlohmann::json::array_t({"Transaction not in progress"})}}));
    return;
  }

  std::vector<std::string> parts;
  string body(request.body());
  boost::split(parts, body, boost::is_any_of("&"));
  string qname, qtype, content;
  uint32_t ttl;

  for (auto const &e : parts) {
    std::vector<std::string> p;
    boost::split(p, e, boost::is_any_of("="));
    if (p.size() != 2) {
      continue;
    }
    if (p.at(0) == "rr[qname]") {
        qname = urlDecode(p.at(1));
    } else if (p.at(0) == "rr[qtype]") {
        qtype = urlDecode(p.at(1));
    } else if (p.at(0) == "rr[content]") {
        content = urlDecode(p.at(1));
    } else if (p.at(0) == "rr[ttl]") {
        ttl = boost::lexical_cast<uint32_t>(urlDecode(p.at(1)));
    }
  }

  it->second->feedRecord(qname, qtype, content, ttl);
  sendResponse(request, response, nlohmann::json({{"result", true}}));
}
}
