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
  /**
   * @brief Construct a new PdnsServerConfig object
   * 
   * This object contains the config for a PowerDNS Authoritative Server,
   * along with some functions to manipulate the stored config.
   * 
   * @param node  A node rooted at '/pdns-server:pdns-server/', used to
   *              extract the PowerDNS configuration
   */
  PdnsServerConfig(const libyang::S_Data_Node &node);

  /**
   * @brief Write a pdns.conf-style file to fpath
   * 
   * @param fpath             File path to write to
   * @throw std::range_error  When the path is invalid
   */
  void writeToFile(const string &fpath);
  void changeConfigValue(const libyang::S_Data_Node_Leaf_List &node);
  void deleteConfigValue();

  /**
   * @brief Get the Webserver Address
   * 
   * @return string 
   */
  std::string getWebserverAddress() {
    return webserver.address + ":" + std::to_string(webserver.port);
  };

  /**
   * @brief Get the Api Key
   * 
   * @return string 
   */
  std::string getApiKey() {
    return webserver.api_key;
  };

/**
 * @brief Whether or not the API is enabled
 * 
 * @return true 
 * @return false 
 */
  bool doesAPI() {
    return webserver.api;
  };

private:
  /**
   * @brief Converts a bool to a string
   * 
   * Returns "true" or "false"
   * 
   * @param b        bool to convert
   * @return string 
   */
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

  struct {
    bool webserver{false};
    string address{"127.0.0.1"};
    uint16_t port{8081};
    string password;
    bool api{false};
    string api_key;
    vector<string> allow_from{{"127.0.0.0/8"}};
    uint32_t max_body_size{2};
    string loglevel{"normal"};
  } webserver;

  vector<listenAddress> listenAddresses{};
  vector<backend> backends;
  bool master;
  bool slave;
};
} // namespace pdns_conf