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

#include "session.hh"

namespace sr
{
Session::~Session() {
  // Somehow the original wrapper does not do this.
  session_stop();
}

void Session::session_stop() {
  if (d_started) {
    sysrepo::Session::session_stop();
  }
}

vector<sysrepo::S_Val> Session::get_items(const string xpath, uint32_t timeout_ms) {
  auto vals = sysrepo::Session::get_items(xpath.c_str(), timeout_ms);
  vector<sysrepo::S_Val> r;
  for (size_t i = 0; i < vals->val_cnt(); i++) {
    r.push_back(vals->val(i));
  }
  return r;
}

vector<sysrepo::S_Val> Session::getConfig(uint32_t timeout_ms) {
  vector<sysrepo::S_Val> r = get_items("/pdns-server:pdns-server/*", timeout_ms);
  return r;
}
} // namespace sr
