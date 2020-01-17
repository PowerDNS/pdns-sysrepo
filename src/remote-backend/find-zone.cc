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

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend {

string RemoteBackend::findBestZone(const string& qname) {
  // I miss dnsname.cc :(
  std::vector<std::string> labels;
  boost::split(labels, qname, boost::is_any_of("."));
  string ret = qname;
  auto session = getSession();

  while (!labels.empty()) {
    try {
      string xpath = fmt::format("/pdns-server:zones/zones[name='{}']", ret);
      spdlog::trace("Attempting to find zone '{}' xpath={}", ret, xpath);
      auto data = session->get_items(xpath.c_str());
      if (data->val_cnt() > 0) {
        spdlog::trace("Found zone '{}'", ret);
        return ret;
      }
      spdlog::trace("not found");
    }
    catch (const sysrepo::sysrepo_exception& e) {
      // Do nothing
    }
    labels.erase(labels.begin());
    ret = boost::algorithm::join(labels, ".");
    if (ret.empty()) {
      ret = ".";
    }
  }
  throw std::out_of_range(fmt::format("No zone found for qname {}", qname));
}
}