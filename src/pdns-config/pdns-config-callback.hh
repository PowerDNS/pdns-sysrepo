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

#include "sysrepo-cpp/Session.hpp"
#include "config/config.hh"
#include "ApiClient.h"
#include "model/Zone.h"
#include "iputils/iputils.hh"

namespace pdns_api = org::openapitools::client::api;
namespace pdns_api_model = org::openapitools::client::model;

namespace pdns_sysrepo::pdns_config {
class PdnsConfigCB : public sysrepo::Callback
{
public:
/**
 * @brief Construct a new ServerConfigCB object
 * 
 * This class implements the sysrepo::Callback specifically for the
 * PowerDNS Authoritative Server. An instance of this should be used in
 * a sysrepo::Subscribe, it then receives the changes in the datastore.
 * Please use pdns_conf::getServerConfigCB to create an instance.
 * 
 * @param privData  Data that the functions can use
 * @param apiClient  apiClient for the PowerDNS API. Set this to a nullptr to indicate
 *                   that PowerDNS will connect to pdns-sysrepo's remote backend.
 */
  PdnsConfigCB(std::shared_ptr<pdns_sysrepo::config::Config> &config, std::shared_ptr<pdns_api::ApiClient> &apiClient) :
    sysrepo::Callback(),
    d_apiClient(apiClient),
    d_config(config)
    {};
  ~PdnsConfigCB(){};

  /**
   * @brief Callback for when data in a module changes.
   * 
   * @see sysrepo::Callback::module_change
   * 
   * @param session 
   * @param module_name 
   * @param xpath 
   * @param event 
   * @param request_id 
   * @param private_data 
   * @return int 
   */
  int module_change(sysrepo::S_Session session, const char* module_name,
    const char* xpath, sr_event_t event,
    uint32_t request_id, void* private_data) override;

protected:
  /**
   * @brief Return a file path to a tempfile
   * 
   * @param request_id  A number that should be unique for the
   *                    whole transaction
   * @return string 
   */
  string tmpFile(const uint32_t request_id);

  /**
   * @brief Restarts a systemd service
   * 
   * This function uses the dbus interface to systemd to call the
   * RestartUnit method. It will then wait to see if the restart succeeded.
   * 
   * @param service  The service to restart
   * @throw std::runtime_error  When communication over dbus fails
   */
  void restartService(const string& service);

  /**
   * @brief Configures d_apiClient
   * 
   * @param node The current config for the server (node rooted at /pdns-server:pdns-server)
   */
  void configureApi(const libyang::S_Data_Node &node); 

  /**
   * @brief API client
   */
  std::shared_ptr<pdns_api::ApiClient> d_apiClient;

  /**
   * @brief Holder for the pdns-sysrepo config
   * 
   * Used to look up the file path and service name
   */
  std::shared_ptr<pdns_sysrepo::config::Config> d_config;

  /**
   * @brief Checks if pdns.conf requires updating
   * 
   * If pdns.conf requires updating, it will create the temporary config file and
   * set pdnsConfigChanged to true.
   * 
   * @param session The session passed to the callback
   * @param request_id The request ID passed to the callback
   */
  void changeConfigUpdate(sysrepo::S_Session &session, const uint32_t request_id);

  /**
   * @brief Handles the additions and removals of zones.
   * 
   * This will add the zones that are removed or added to the zonesCreated and zonesRemoved
   * lists
   * 
   * @param sess    The session passed to the callback, used to get data if required
   * 
   * @throw std::runtime_error  When an addition or removal is not possible
   */
  void changeZoneAddAndDelete(sysrepo::S_Session &sess);

  /**
   * @brief Uses the API to add and remove zones
   * 
   * @throw std::runtime_error  When one or more zones can't be added/deleted
   */
  void doneZoneAddAndDelete();

  void changeZoneModify(sysrepo::S_Session &session);
  void doneZoneModify();

  /**
   * @brief Get the ID of a zone
   * 
   * @param zone 
   * @return string 
   * @throw runtime_error when the API does not return what we need
   * @throw org::openapitools::client::api::ApiException when the API does not return with 2xx
   */
  string getZoneId(const string &zone);

  std::vector<pdns_api_model::Zone> zonesCreated;
  std::map<string, pdns_api_model::Zone> zonesModified;
  std::vector<string> zonesRemoved;
  bool pdnsConfigChanged{false};
  bool apiConfigChanged{false};
  bool isFromEnabled{false};
};

/**
 * @brief Get a shared pointer to a ServerConfigCB object
 * 
 * @param config                A shared_ptr to the pdns-sysrepo config holder
 * @param apiClient             A shared_ptr to an API Client
 * @return std::shared_ptr<ServerConfigCB> 
 */
std::shared_ptr<PdnsConfigCB> getServerConfigCB(std::shared_ptr<pdns_sysrepo::config::Config> &config, std::shared_ptr<pdns_api::ApiClient> &apiClient);

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
  void writeToFile(const string &fpath);
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
    string password;
    bool api{false};
    string api_key;
    std::vector<std::string> allow_from{{"127.0.0.0/8"}};
    uint32_t max_body_size{2};
    string loglevel{"normal"};
  } webserver;

  std::vector<listenAddress> listenAddresses{};
  std::vector<backend> backends;
  bool master{false};
  bool slave{false};

  std::vector<axfrAcl> allowAxfrIps;
  std::vector<axfrAcl> alsoNotifyIps;
};
}