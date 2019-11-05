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

#include <sysrepo-cpp/Session.hpp>
#include <spdlog/spdlog.h>
#include <sdbusplus/message.hpp>

#include "configurator.hh"

using namespace std;

namespace pdns_conf
{
sysrepo::S_Callback getServerConfigCB(const string& fpath, const string& serviceName);

class ServerConfigCB : public sysrepo::Callback
{
public:
  ServerConfigCB(const map<string, string>& privData) :
    sysrepo::Callback(),
    privData(privData){};
  ~ServerConfigCB(){};
  int module_change(sysrepo::S_Session session, const char* module_name,
    const char* xpath, sr_event_t event,
    uint32_t request_id, void* private_data) override;

private:
  string tmpFile(const uint32_t request_id);
  void restartService(const string& service);

  map<string, string> privData;
  vector<string> sdJobs;

  // TODO figure out if we actually need this
  shared_ptr<PdnsServerConfig> pdnsServerConfig;
};
} // namespace pdns_conf