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
#include <libyang/Libyang.hpp>

#include "config.hh"

namespace pdns_sysrepo::config
{
void Config::setLoglevel(const string& level) {
  spdlog::set_level(spdlog::level::from_str(level));
}

void Config::setLogFormat(const bool& timestamped) {
  if (timestamped) {
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%f - %l - %v");
    return;
  }
    spdlog::set_pattern("%l - %v");
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
      setLogFormat(getOption("logging/timestamp", session) == "true");
      d_pdns_service = getOption("pdns-service/name", session);
      d_pdns_conf = getOption("pdns-service/config-file", session);
    }
    catch (const sysrepo::sysrepo_exception& e) {
      spdlog::warn("Unable to retrieve initial config due to sysrepo error: {}", e.what());
      d_failed = true;
      d_initial_done = true;
      return SR_ERR_OPERATION_FAILED;
    }
    catch (const std::exception& e) {
      spdlog::warn("Unable to retrieve initial config: {}", e.what());
      d_failed = true;
      d_initial_done = true;
      return SR_ERR_OPERATION_FAILED;
    }
    d_initial_done = true;
  }

  if (event == SR_EV_DONE) {
    if (d_skip_done) {
      d_skip_done = false;
      return SR_ERR_OK;
    }
    try {
      auto changeIter = session->get_changes_iter("/pdns-server:pdns-sysrepo//*");
      sysrepo::S_Tree_Change change;
      while (change = session->get_change_tree_next(changeIter)) {
        if (change->node()->schema()->nodetype() == LYS_LEAF) {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(change->node());
          spdlog::trace("pdns-sysrepo config change path={} value={}", leaf->path(), leaf->value_str());
          string path(change->node()->path());
          if (path == "/pdns-server:pdns-sysrepo/pdns-service/config-file") {
            d_pdns_conf = leaf->value_str();
          } else if (path == "/pdns-server:pdns-sysrepo/pdns-service/name") {
            d_pdns_service = leaf->value_str();
          } else if (path == "/pdns-server:pdns-sysrepo/logging/level") {
            setLoglevel(leaf->value_str());
          } else if (path == "/pdns-server:pdns-sysrepo/logging/timestamp") {
            setLogFormat(leaf->value()->bln());
          }
        }
      }
    }
    catch(const sysrepo::sysrepo_exception &e) {
      spdlog::warn("Error from sysrepo while updating pdns-sysrepo config: {}", e.what());
      auto errs = session->get_error();
      for (size_t i = 0; i < errs->error_cnt(); i++) {
        spdlog::error("  Error from sysrepo session: {}", errs->message(i));
      }
      return SR_ERR_OPERATION_FAILED;
    }
  }
  return SR_ERR_OK;
}
} // namespace pdns_sysrepo::config