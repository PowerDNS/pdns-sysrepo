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

libyang::S_Data_Node Session::getConfigTree(uint32_t timeout_ms) {
  return get_subtree("/pdns-server:pdns-server/.", timeout_ms);
}
} // namespace sr
