/**
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

#include <iostream>

#include <spdlog/spdlog.h>

#include "sr_wrapper/session.hh"

using namespace std;

/*
 * # The grand plan
 * 
 * This program will subscribe to a "dns-server" and a "pdns-server" YANG model installed in sysrepo.
 * These models will have all the config options from PowerDNS and a way to express the zones with their configuration.
 * For the zones we will 'take over' the part of the tree and use the PowerDNS API to query the zone status and config
 * 
 * ## Service configuration changes:
 * When the configuration changes, this program will write a new pdns.conf and tell systemd over dbus to restart the service
 * 
 * ## Zone changes
 * The API will be used for these changes
 * 
 */

int main() {
  try {
    spdlog::info("Starting Connection");
    sysrepo::S_Connection conn(new sysrepo::Connection());

    spdlog::info("Starting session");
    auto sess = sr::Session(conn);
    string xpath = "/pdns-server:pdns-server/listen-addresses/*";

    spdlog::info("getting items at {}", xpath);
    try {
      auto values = sess.get_items(xpath);
      for (const auto &v : values) {
        spdlog::info("item xpath={} type={}", v->xpath(), v->type());
      }
    }
    catch (const std::exception& e) {
      auto errs = sess.get_error();
      for (size_t i=0; i<errs->error_cnt(); i++) {
        spdlog::error("Had an error from session: {}", errs->message(i));
      }
    }
  } catch( const std::exception& e ) {
    spdlog::error("Had an error: {}", e.what());
    return 1;
  }
  return 0;
}