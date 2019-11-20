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

#include <boost/filesystem.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include "subscribe.hh"
#include "configurator.hh"
#include "util.hh"
#include "sr_wrapper/session.hh"
#include "api/ZonesApi.h"
#include "model/Zone.h"

namespace fs = boost::filesystem;
namespace pdns_model = org::openapitools::client::model;

namespace pdns_conf
{
std::shared_ptr<ServerConfigCB> getServerConfigCB(const string& fpath, const string &serviceName) {
  auto cb = make_shared<ServerConfigCB>(ServerConfigCB(
    {{"fpath", fpath},
      {"service", serviceName}}));
  return cb;
}

std::shared_ptr<ZoneCB> getZoneCB(const string &url, const string &passwd) {
  auto cb = make_shared<ZoneCB>(ZoneCB(url, passwd));
  return cb;
}

string ServerConfigCB::tmpFile(const uint32_t request_id) {
  return privData["fpath"] + "-tmp-" + to_string(request_id);
}

int ServerConfigCB::module_change(sysrepo::S_Session session, const char* module_name,
  const char* xpath, sr_event_t event,
  uint32_t request_id, void* private_data) {
  spdlog::trace("Had callback. module_name={} xpath={} event={} request_id={}",
    (module_name == nullptr) ? "" : module_name,
    (xpath == nullptr) ? "<none>" : xpath,
    util::srEvent2String(event), request_id);

  if (event == SR_EV_CHANGE) {
    auto fpath = tmpFile(request_id);
    auto sess = static_pointer_cast<sr::Session>(session);

    // The session already has the new datastore values
    PdnsServerConfig c(sess->getConfigTree());

    c.writeToFile(fpath);
  }

  if (event == SR_EV_DONE) {
    auto fpath = tmpFile(request_id);

    auto sess = static_pointer_cast<sr::Session>(session);
    try {
      spdlog::debug("Moving {} to {}", fpath, privData["fpath"]);
      fs::rename(fpath, privData["fpath"]);
    } catch (const exception &e) {
      spdlog::warn("Unable to move {} to {}: {}", fpath, privData["fpath"], e.what());
      return SR_ERR_OPERATION_FAILED;
    }

    try {
      // TODO Are we responsible for this, or should we let the network service controller decide?
      session->copy_config(SR_DS_RUNNING, SR_DS_STARTUP, module_name);
    }
    catch (const sysrepo::sysrepo_exception& se) {
      spdlog::warn("Could not copy running config to startup config");
      return SR_ERR_OPERATION_FAILED;
    }
    if (!privData["service"].empty()) {
      restartService(privData["service"]);
    }
    if (d_zoneCB != nullptr) {
      PdnsServerConfig c(sess->getConfigTree());
      d_zoneCB->setApiKey(c.getApiKey());
    }
  }

  if (event == SR_EV_ABORT) {
    auto fpath = tmpFile(request_id);
    try {
      fs::remove(fpath);
    }
    catch (const exception& e) {
      spdlog::warn("Unable to remove temporary file fpath={}: {}", fpath, e.what());
    }
  }
  return SR_ERR_OK;
}

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
    } catch (const sdbusplus::exception::exception &e) {
      throw runtime_error("Could not request service restart: " + string(e.description()));
    }

    try {
      sdbusplus::message::object_path job;
      reply.read(job);
      spdlog::debug("restart requested job={}", job.str);
    } catch (const sdbusplus::exception_t &e) {
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
  catch (const runtime_error &e) {
    spdlog::warn("Error restarting: {}", e.what());
  }
}

int ZoneCB::oper_get_items(sysrepo::S_Session session, const char* module_name,
  const char* path, const char* request_xpath,
  uint32_t request_id, libyang::S_Data_Node& parent, void* private_data) {
  spdlog::trace("oper_get_items called path={} request_xpath={}",
    path, (request_xpath != nullptr) ? request_xpath : "<none>");

  libyang::S_Context ctx = session->get_context();
  libyang::S_Module mod = ctx->get_module(module_name);
  parent.reset(new libyang::Data_Node(ctx, "/pdns-server:zones-state", nullptr, LYD_ANYDATA_CONSTSTRING, 0));

  pdns_api::ZonesApi zoneApiClient(d_apiClient);
  try {
    // We use a blocking call
    auto res = zoneApiClient.listZones("localhost", boost::none);
    auto zones = res.get();
    for (const auto &zone : zones) {
      libyang::S_Data_Node zoneNode(new libyang::Data_Node(parent, mod, "zones"));
      libyang::S_Data_Node nameNode(new libyang::Data_Node(zoneNode, mod, "name", zone->getName().c_str()));
      libyang::S_Data_Node serialNode(new libyang::Data_Node(zoneNode, mod, "serial", to_string(zone->getSerial()).c_str()));
    }
  } catch (const web::uri_exception &e) {
    spdlog::warn("Unable to retrieve zones from from server: {}", e.what());
  } catch (const org::openapitools::client::api::ApiException &e) {
    spdlog::warn("Unable to retrieve zones from from server: {}", e.what());
  }

  return SR_ERR_OK;
}

void ZoneCB::setApiKey(const string &apiKey) {
  d_apiClient->getConfiguration()->setApiKey("X-API-Key", apiKey);
}

} // namespace pdns_conf