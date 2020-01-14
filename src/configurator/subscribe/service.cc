/*
 * Copyright 2019-2020 Pieter Lexis <pieter.lexis@powerdns.com>
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
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include "../subscribe.hh"

namespace pdns_conf
{
void ServerConfigCB::restartService(const string& service) {
  bool hadSignal = false;
  sdbusplus::message::message signalMessage;
  auto signalInterface = sdbusplus::bus::match::rules::interface("org.freedesktop.systemd1.Manager");
  auto signalMember = sdbusplus::bus::match::rules::member("JobRemoved");

  auto sdJobCB = [&hadSignal, &signalMessage](sdbusplus::message::message& m) {
    hadSignal = true;
    signalMessage = m;
  };

  spdlog::debug("Attempting to restart {}", service);
  try {
    auto b = sdbusplus::bus::new_default_system();

    sdbusplus::bus::match_t tmp(b,
      sdbusplus::bus::match::rules::type::signal() + signalInterface + signalMember,
      sdJobCB);

    sdbusplus::message::message reply;

    try {
      auto m = b.new_method_call("org.freedesktop.systemd1",
        "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager",
        "RestartUnit");
      m.append(service, "replace");
      reply = b.call(m);
    }
    catch (const sdbusplus::exception::exception& e) {
      throw runtime_error("Could not request service restart: " + string(e.description()));
    }

    try {
      sdbusplus::message::object_path job;
      reply.read(job);
      spdlog::debug("restart requested job={}", job.str);
    }
    catch (const sdbusplus::exception_t& e) {
      throw runtime_error("Problem getting reply: " + string(e.description()));
    }

    // Wait for the signal to come back
    for (size_t i = 0; i < 15 && !hadSignal; i++) {
      b.wait(200000); // microseconds, so maximum 3 seconds
      b.process_discard();
    }

    if (hadSignal) {
      spdlog::debug("Received signal from dbus for a job change");

      // TODO handle errors with restarting etc.

      uint32_t id;
      sdbusplus::message::object_path opath;
      string servicename;
      string result;
      try {
        signalMessage.read(id, opath, servicename, result);
        spdlog::debug("Had signal for job {}, service {}, result: {}", opath.str, servicename, result);
      }
      catch (const sdbusplus::exception_t& e) {
        spdlog::warn("Unable to parse dbus message for a finished job: {}", e.description());
      }
    }
    else {
      spdlog::debug("no signal....");
    }
  }
  catch (const sdbusplus::exception_t& e) {
    spdlog::warn("Could not communicate to dbus: {}", e.description());
  }
  catch (const runtime_error& e) {
    spdlog::warn("Error restarting: {}", e.what());
  }
}

} // namespace pdns_conf