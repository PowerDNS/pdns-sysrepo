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
#include <set>
#include <signal.h>

#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>
#include <fmt/format.h>
#include <systemd/sd-daemon.h>
#include <pistache/net.h>

#include "sr_wrapper/session.hh"
#include "configurator/subscribe.hh"
#include "configurator/configurator.hh"
#include "config/config.hh"
#include "config.h"
#include "remote-backend/remote-backend.hh"

using std::set;
using std::cout;
namespace po = boost::program_options;

static bool doExit = false;

static void siginthandler(int signum) {
  if (doExit) {
    spdlog::warn("Forced exit requested");
    exit(1 + signum);
  }
  doExit = true;
}

int main(int argc, char* argv[]) {
  po::options_description opts("Options");

  opts.add_options()
    ("help,h", "Output a help message")
    ("version,v", "Show the version");

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
  }
  catch (const boost::program_options::error& e) {
    cerr << fmt::format("Unable to parse commandline arguments: {}", e.what()) << endl;
    return 1;
  }

  if (vm.count("help")) {
    cout << opts << endl;
    return 0;
  }

  if (vm.count("version")) {
    cout << VERSION << endl;
    return 0;
  }

  try {
    sysrepo::S_Connection conn(new sysrepo::Connection());

    auto sess = sr::Session(conn);

    try {
      /* get our own settings first */
      auto configSession(make_shared<sysrepo::Session>(sess));
      auto configSubscription = make_shared<sysrepo::Subscribe>(configSession);
      auto myConfig = make_shared<pdns_sysrepo::config::Config>();
      configSubscription->module_change_subscribe("pdns-server", myConfig, "/pdns-server:pdns-sysrepo", nullptr, 0, SR_SUBSCR_ENABLED);
      for (size_t tries = 0; tries < 10 && !myConfig->finished(); tries++) {
        sleep(1);
      }
      if (!myConfig->finished()) {
        throw std::runtime_error("Unable to retrieve my own configuration from sysrepo");
      }
      if (myConfig->failed()) {
        throw std::runtime_error("Errors while processing initial config for pdns-sysrepo");
      }
      spdlog::debug("Configuration complete, starting callbacks for pdns-server");

      /* This is passed to both the ServerConfigCB and the ZoneCB.
         As the have the same reference, the ServerConfigCB can update the pdns_api::ApiClient with the
         correct config, allowing both the ZoneCB and the ServerConfigCB to work with the API
      */
      auto apiClient = make_shared<pdns_api::ApiClient>();

      spdlog::debug("Registering config change callbacks");
      sysrepo::S_Session sSess(make_shared<sysrepo::Session>(sess));
      auto s = sysrepo::Subscribe(sSess);
      auto cb = pdns_conf::getServerConfigCB(myConfig, apiClient);
      s.module_change_subscribe("pdns-server", cb, nullptr, nullptr, 0, SR_SUBSCR_ENABLED);

      auto zoneSubscribe = sysrepo::Subscribe(sSess);
      spdlog::debug("done, registring operational zone CB");
      auto zoneCB = pdns_conf::getZoneCB(apiClient);
      zoneSubscribe.oper_get_items_subscribe("pdns-server", "/pdns-server:zones-state", zoneCB);

      spdlog::trace("callbacks registered, registring signal handlers");

      signal(SIGINT, siginthandler);
      signal(SIGSTOP, siginthandler);
      spdlog::trace("signalhandlers registered, notifying systemd we're ready");

      sd_notify(0, "READY=1");
      spdlog::info("Starting remote backend webserver");
      
      Pistache::Address a = "127.0.0.1:9100";
      auto rb = pdns_sysrepo::remote_backend::RemoteBackend(sSess, a);
      rb.start();

      spdlog::info("Startup complete");

      while (!doExit) {
        sleep(500);
      }

      spdlog::info("Exit requested");
      sd_notify(0, "STOPPING=1");
    }
    catch (const sysrepo::sysrepo_exception& e) {
      auto errs = sess.get_error();
      for (size_t i = 0; i < errs->error_cnt(); i++) {
        spdlog::error("Had an error from session: {}", errs->message(i));
      }
      throw;
    }
  }
  catch (const sysrepo::sysrepo_exception& e) {
    spdlog::error("Had an error from sysrepo: {}", e.what());
    return 1;
  }
  catch (const std::exception& e) {
    spdlog::error("Fatal error: {}", e.what());
    return 1;
  }
  return 0;
}
