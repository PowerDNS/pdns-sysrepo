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

#include "spdlog/spdlog.h"

#include "session.hh"

namespace sr
{
Session::~Session() {
  // Somehow the original wrapper does not do this.
  session_stop();
}

libyang::S_Data_Node Session::getConfigTree(uint32_t timeout_ms) {
  return get_subtree("/pdns-server:pdns-server/.", timeout_ms);
}

vector<string> Session::getZoneMasters(const string &zone, uint32_t timeout_ms) {
  string path = fmt::format("/pdns-server:zones[name='{}']/pdns-server:masters", zone);
  spdlog::trace("getZoneMasters({}), path={}", zone, path);

  vector<string> ret;
  try {
    auto mastersSet = get_subtree("/pdns-server:zones", timeout_ms)->find_path(path.c_str());
    for (auto const& n : mastersSet->data()) {
      if (n->schema()->nodetype() == LYS_LEAFLIST) {
        auto l = make_shared<libyang::Data_Node_Leaf_List>(n);
        ret.push_back(l->value_str());
      }
    }
  } catch (const sysrepo::sysrepo_exception &e) {
    sysrepo::S_Errors errors = get_error();
    for (size_t i = 0; i < errors->error_cnt(); i++) {
      spdlog::warn("Unable to get zone masters: {}", errors->message(i));
    }
  }
  return ret;
}
} // namespace sr
