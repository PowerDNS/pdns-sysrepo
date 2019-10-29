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
#include <fmt/core.h>
#include <systemd/sd-daemon.h>

#include "sr_wrapper/session.hh"
#include "configurator/subscribe.hh"
#include "configurator/configurator.hh"
#include "configurator/config.hh"

using namespace std;
namespace po = boost::program_options;

static const set<string> logLevels{"trace", "debug", "info", "warning", "error", "critical", "off"};

static bool doExit = false;

static void siginthandler(int signum) {
  if (doExit) {
    spdlog::warn("Forced exit requested");
    exit(1 + signum);
  }
  doExit = true;
}

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

int main(int argc, char* argv[]) {
  po::options_description opts("Options");
  string logLevelHelp = fmt::format("The loglevel of the program, possible values are {}", fmt::join(logLevels, ", "));

  opts.add_options()("help,h", "Output a help message")("config,c", po::value<string>()->default_value("/etc/pdns-configurator/pdns-configurator.yaml"), "Configuration file to load.")("loglevel,l", po::value<string>()->default_value("info"), logLevelHelp.c_str());

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

  if (logLevels.count(vm["loglevel"].as<string>()) == 0) {
    spdlog::error("Unknown loglevel {}", vm["loglevel"].as<string>());
    return 1;
  }
  spdlog::set_level(spdlog::level::from_str(vm["loglevel"].as<string>()));
  spdlog::info("Level set to {}", vm["loglevel"].as<string>());

  try {
    auto myConfig = pdns_conf::Config(vm["config"].as<string>());

    spdlog::info("Starting Connection");
    sysrepo::S_Connection conn(new sysrepo::Connection());

    spdlog::info("Starting session");
    auto sess = sr::Session(conn);

    spdlog::info("Getting current config");
    try {
      auto values = sess.getConfig();
      spdlog::info("Writing config to {}", myConfig.getPdnsConfigFilename());
      pdns_conf::writeConfig(myConfig.getPdnsConfigFilename(), values);
      spdlog::debug("Writing done");

      spdlog::debug("Registring config change callbacks");
      auto s = sysrepo::Subscribe(make_shared<sysrepo::Session>(sess));
      auto cb = pdns_conf::getServerConfigCB(myConfig.getPdnsConfigFilename());
      s.module_change_subscribe("pdns-server", cb);
      spdlog::trace("callbacks registered, registring signal handlers");
      signal(SIGINT, siginthandler);
      signal(SIGSTOP, siginthandler);
      spdlog::trace("signalhandlers registered, notifying systemd we're ready");
      sd_notify(0, "READY=1");
      spdlog::info("Startup complete");
      while (!doExit) {
        sleep(500);
      }
      spdlog::info("Exit requested");
      sd_notify(0, "STOPPING=1");
      // Session::session_stop() seems to hang
      // sess.session_stop();
    }
    catch (const sysrepo::sysrepo_exception& e) {
      auto errs = sess.get_error();
      for (size_t i = 0; i < errs->error_cnt(); i++) {
        spdlog::error("Had an error from session: {}", errs->message(i));
      }
      throw;
    }
  }
  catch (const std::exception& e) {
    spdlog::error("Fatal error: {}", e.what());
    return 1;
  }
  return 0;
}