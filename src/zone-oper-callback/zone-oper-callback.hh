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

#include <sysrepo-cpp/Session.hpp>

#include "ApiClient.h"

namespace pdns_api = org::openapitools::client::api;
namespace pdns_api_model = org::openapitools::client::model;

namespace pdns_sysrepo::zone_oper_callback
{
class ZoneCB : public sysrepo::Callback
{
  public:
  ZoneCB(std::shared_ptr<pdns_api::ApiClient> &apiClient) :
    sysrepo::Callback(),
    d_apiClient(apiClient)
    {};
  ~ZoneCB(){};

  /**
   * @brief Callback called when an application is requesting operational or config data
   * 
   * @see sysrepo::Callback::oper_get_items
   * 
   * @param session 
   * @param module_name 
   * @param path 
   * @param request_xpath 
   * @param request_id 
   * @param parent 
   * @param private_data 
   * @return int 
   */
  int oper_get_items(sysrepo::S_Session session, const char* module_name,
    const char* path, const char* request_xpath,
    uint32_t request_id, libyang::S_Data_Node& parent, void* private_data) override;

  private:
  std::shared_ptr<pdns_api::ApiClient> d_apiClient;
};


/**
 * @brief Get a new ZoneCB object wrapped in a shared_ptr
 * 
 * @param url     Domain name for the API, "http://" wil be prepended, "/api/v1" will be appended
 * @param passwd  The API Key to use
 * @return std::shared_ptr<ZoneCB> 
 */
std::shared_ptr<ZoneCB> getZoneCB(std::shared_ptr<pdns_api::ApiClient> &apiClient);

} // namespace pdns_conf