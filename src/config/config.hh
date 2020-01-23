/*
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
#include <string>
#include <sysrepo-cpp/Session.hpp>

using std::string;

namespace pdns_sysrepo::config
{
class Config : public sysrepo::Callback
{
public:
  /**
   * @brief Get the path to the PowerDNS Authoritative Server config file
   * 
   * @return string 
   */
  string getPdnsConfigFilename() { return d_pdns_conf; };

  /**
   * @brief Get the name of the PowerDNS service
   * 
   * @return string
   */
  string getServiceName() { return d_pdns_service; };

  /**
   * @brief Whether or not the PowerDNS service should be restarted
   * 
   * @return true 
   * @return false 
   */
  bool getDoServiceRestart() { return d_pdns_restart; };

  int module_change(sysrepo::S_Session session, const char* module_name,
    const char* xpath, sr_event_t event,
    uint32_t request_id, void* private_data) override;

  bool failed() {
    return d_failed;
  }

  bool finished() {
    return d_initial_done;
  }

private:
  void setLoglevel(const string& level);
  void setLogFormat(const bool& timestamped);

  /**
   * @brief Get the option named opt
   * 
   * @param opt 
   * @param session 
   * @return string 
   */
  string getOption(const string &opt, sysrepo::S_Session &session);
  string d_pdns_service;
  string d_pdns_conf;

  /**
   * @brief Whether or not the PowerDNS service should be restarted on config changes
   * 
   */
  bool d_pdns_restart;

  /**
   * @brief Indicates that the initial load failed
   */
  bool d_failed = false;

  /**
   * @brief Indicates that the SR_EV_ENABLE was received and properly processed
   */
  bool d_initial_done = false;

  /**
   * @brief Ensures the first SR_EV_DONE is skipped
   */
  bool d_skip_done = true;
};
} // namespace pdns_conf