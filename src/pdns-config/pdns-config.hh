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
#pragma once

#include "iputils/iputils.hh"
#include "libyang/Libyang.hpp"
#include "sysrepo-cpp/Sysrepo.hpp"

namespace pdns_sysrepo::pdns_config {
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
   * @param session  A sysrepo session that is used to retrieve the config
   * @param doRemoteBackendOnly  Configure a single remote backend to connect to pdns-sysrepo
   */
  PdnsServerConfig(const libyang::S_Data_Node &node, const sysrepo::S_Session &session = nullptr, const bool doRemoteBackendOnly = false);

  /**
   * @brief Write a pdns.conf-style file to fpath
   * 
   * @param fpath             File path to write to
   * @throw std::range_error  When the path is invalid
   */
  void writeToFile(const std::string &fpath);
  void changeConfigValue(const libyang::S_Data_Node_Leaf_List &node);
  void deleteConfigValue();

  std::string getConfig();

  /**
   * @brief Get the Webserver Address
   * 
   * @return string 
   */
  std::string getWebserverAddress() {
    return webserver.address.toStringWithPort();
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
  std::string bool2str(const bool b);

  struct listenAddress {
      std::string name;
      iputils::ComboAddress address;
  };

  struct axfrAcl {
    std::string name;
    std::vector<std::string> addresses; // TODO migrate to iputil::NetMask
  };

  struct backend {
    std::string name;
    std::string backendtype;
    std::vector<std::pair<std::string, std::string>> options{};
  };

  struct {
    bool webserver{false};
    iputils::ComboAddress address{"127.0.0.1:8081"};
    std::string password;
    bool api{false};
    std::string api_key;
    std::vector<std::string> allow_from{{"127.0.0.0/8"}};
    uint32_t max_body_size{2};
    std::string loglevel{"normal"};
  } webserver;

  std::vector<listenAddress> listenAddresses{};
  std::vector<backend> backends;
  bool master{false};
  bool slave{false};

  std::vector<axfrAcl> allowAxfrIps;
  std::vector<axfrAcl> alsoNotifyIps;
};
}