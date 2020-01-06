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

#include <spdlog/spdlog.h>

#include "config.hh"

namespace pdns_sysrepo::config
{
void Config::setLoglevel(const string& level) {
  spdlog::set_level(spdlog::level::from_str(level));
}

string Config::getOption(const string &opt, sysrepo::S_Session &session) {
  string path = fmt::format("/pdns-server:pdns-sysrepo/{}", opt);
  auto item = session->get_item(path.c_str(), 3);
  auto ret = item->val_to_string();
  if (ret.empty()) {
    throw std::out_of_range(fmt::format("Could not find an option for pdns-sysrepo named '{}'", opt));
  }
  return ret;
}

int Config::module_change(sysrepo::S_Session session, const char* module_name,
  const char* xpath, sr_event_t event,
  uint32_t request_id, void* private_data) {

  if (event == SR_EV_ENABLED) {
    spdlog::set_pattern("%l - %v");
    try {
      setLoglevel(getOption("logging/level", session));
      auto timestamp = getOption("logging/timestamp", session);
      if (timestamp == "true") {
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%f - %l - %v");
      }
      d_pdns_service = getOption("pdns-service/name", session);
      d_pdns_conf = getOption("pdns-service/config-file", session);
    }
    catch (const std::exception& e) {
      d_failed = true;
      d_initial_done = true;
      return SR_ERR_OPERATION_FAILED;
    }
    d_initial_done = true;
  }
  return SR_ERR_OK;
}
} // namespace pdns_sysrepo::config