/**
 * Copyright 2019-2020 Pieter Lexis <pieter.lexis@powerdns.com>
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
#include <fmt/format.h>

#include "remote-backend.hh"
#include <spdlog/spdlog.h>

namespace pdns_sysrepo::remote_backend {

using std::vector;

vector<string> split(const string& str, const string& delim) {
  vector<string> tokens;
  size_t prev = 0, pos = 0;
  do {
    pos = str.find(delim, prev);
    if (pos == string::npos)
      pos = str.length();
    string token = str.substr(prev, pos - prev);
    if (!token.empty())
      tokens.push_back(token);
    prev = pos + delim.length();
  } while (pos < str.length() && prev < str.length());
  return tokens;
}

string RemoteBackend::findBestZone(const string& qname) {
  // I miss dnsname.cc :(
  auto labels = split(qname, ".");
  string ret = qname;
  auto session = getSession();

  // haha, no root support
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
    ret = boost::algorithm::join(labels, ".") + ".";
  }
  throw std::out_of_range(fmt::format("No zone found for qname {}", qname));
}
}