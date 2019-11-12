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
#pragma once

#include <string>
#include <vector>

#include <sysrepo-cpp/Sysrepo.hpp>
#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>

using std::vector;
using std::string;
using std::pair;

namespace pdns_conf
{
class PdnsServerConfig
{
public:
  PdnsServerConfig(const libyang::S_Data_Node &node);
  void writeToFile(const string &fpath);
  void changeConfigValue(const libyang::S_Data_Node_Leaf_List &node);
  void deleteConfigValue();

private:
  string bool2str(const bool b);
  struct listenAddress {
      string name;
      string address;
      uint16_t port;
  };

  struct backend {
    string name;
    string backendtype;
    vector<pair<string, string>> options{};
  };

  vector<listenAddress> listenAddresses{};
  vector<backend> backends;
  bool master;
  bool slave;
};
} // namespace pdns_conf