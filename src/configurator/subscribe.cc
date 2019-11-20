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
std::shared_ptr<ServerConfigCB> getServerConfigCB(const string& fpath, const string &serviceName, shared_ptr<pdns_api::ApiClient> &apiClient) {
  auto cb = make_shared<ServerConfigCB>(ServerConfigCB(
    {{"fpath", fpath},
      {"service", serviceName}},
    apiClient));
  return cb;
}

std::shared_ptr<ZoneCB> getZoneCB(shared_ptr<pdns_api::ApiClient> &apiClient) {
  auto cb = make_shared<ZoneCB>(ZoneCB(apiClient));
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
    (xpath == nullptr) ? "<nozones change. operation=ne>" : xpath,
    util::srEvent2String(event), request_id);

  if (event == SR_EV_CHANGE) {
    auto fpath = tmpFile(request_id);
    auto sess = static_pointer_cast<sr::Session>(session);

    // This fetches only created and deleted zones
    auto iter = session->get_changes_iter("/pdns-server:pdns-server/zones");
    auto change = session->get_change_tree_next(iter);

    if (d_apiClient == nullptr && change != nullptr) {
      spdlog::error("Unable to change zone configuration, API not enabled.");
      return SR_ERR_OPERATION_FAILED;
    }

    pdns_api::ZonesApi zoneApiClient(d_apiClient);

    while (change != nullptr && change->node() != nullptr) {
      // spdlog::trace("zones change. operation={}", util::srChangeOper2String(change->oper()));

      if (change->oper() == SR_OP_CREATED) {
        auto tree = sess->get_subtree(("/pdns-server:pdns-server" + change->node()->path()).c_str());
        auto child = tree->child();
        pdns_api::Zone z;
        while (child) {
          auto leaf = make_shared<libyang::Data_Node_Leaf_List>(child);
          // spdlog::trace("child name={} value={}", leaf->schema()->name(), leaf->value_str());
          if (string(leaf->schema()->name()) == "name") {
            z.setName(leaf->value_str());
          }
          if (string(leaf->schema()->name()) == "zonetype") {
            z.setKind(leaf->value_str());
          }
          if (string(leaf->schema()->name()) == "class" && !(string(leaf->value_str()) != "IN" || string(leaf->value_str()) != "1")) {
            spdlog::warn("Zone {} can't be validated, class is not IN but {}", z.getName(), leaf->value_str());
            return SR_ERR_VALIDATION_FAILED;
          }
          child = child->next();
        }
        zonesCreated.push_back(z);
      }

      if (change->oper() == SR_OP_DELETED) {
        if (change->node()->schema()->nodetype() != LYS_LIST) {
          spdlog::debug("Had unexpected element type {} at {} while expecting a list for a zone removal",
            util::libyangNodeType2String(change->node()->schema()->nodetype()),
            change->node()->path());
            continue;
        }
        auto child = change->node()->child();
        bool found = false;
        while (child && !found) {
          if (child->schema()->nodetype() == LYS_LEAF && string(child->schema()->name()) == "name") {
            found = true;
            auto l = make_shared<libyang::Data_Node_Leaf_List>(child);
            zonesRemoved.push_back(l->value_str());
          }
          child = child->next();
        }
      }

      if (change->oper() == SR_OP_MODIFIED) {
        // TODO calls to the API for changes
        /*
        auto haveSchema = (change->node()->schema() != nullptr);
        if (haveSchema) {
          spdlog::trace("have node->schema type={} name={}", util::libyangNodeType2String(change->node()->schema()->nodetype()), change->node()->schema()->name());
          if (change->oper() == SR_OP_MODIFIED) {
            auto leaf = make_shared<libyang::Data_Node_Leaf_List>(change->node());
            spdlog::trace("path={} node->path={}", leaf->path(), change->node()->path());
            auto prev_leaf = make_shared<libyang::Data_Node_Leaf_List>(leaf->prev());
            if (prev_leaf) {
              spdlog::trace("leaf->schema->prev->name={} val={}", prev_leaf->schema()->name(), prev_leaf->value_str());
            }
          }
        }
        */
      }
      change = session->get_change_tree_next(iter);
    }
    
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
    if (d_apiClient != nullptr) {
      // XXX is this needed?
      PdnsServerConfig c(sess->getConfigTree());
      d_apiClient->getConfiguration()->setApiKey("X-API-Key", c.getApiKey());

      pdns_api::ZonesApi zonesApiClient(d_apiClient);

      for (auto const &z : zonesCreated) {
        try {
          auto zp = make_shared<pdns_api_model::Zone>(z);
          spdlog::debug("Creating zone {}", zp->getName());
          auto zone = zonesApiClient.createZone("localhost", zp, false).get();
          spdlog::debug("Created zone {}", zp->getName());
        } catch (const pdns_api::ApiException &e) {
          spdlog::error("Unable to create zone '{}': {}", z.getName(), e.what());
        } catch (const std::runtime_error &e) {
          spdlog::error("Unable to create zone '{}': {}", z.getName(), e.what());
        }
      }

      for (auto const &z : zonesRemoved) {
        try {
          auto zones = zonesApiClient.listZones("localhost", z).get();
          if (zones.size() != 1) {
            spdlog::warn("While deleting, API returned the wrong number of zones ({}) for {}, expected 1", zones.size(), z);
            continue;
          }
          auto zone = zones.at(0);
          spdlog::debug("Deleting zone {}", z);
          zonesApiClient.deleteZone("localhost", zone->getId());
          spdlog::debug("Deleted zone {}", z);
        } catch (const pdns_api::ApiException &e) {
          spdlog::error("Unable to delete zone '{}': {}", z, e.what());
        } catch (const std::runtime_error &e) {
          spdlog::error("Unable to delete zone '{}': {}", z, e.what());
        }
      }
    }
    // TODO Store zones that could not be created or deleted and try again later, returning SR_ERR_CALLBACK_SHELVE and starting a thread to try the creation again
    zonesCreated.clear();
    zonesRemoved.clear();
  }

  if (event == SR_EV_ABORT) {
    auto fpath = tmpFile(request_id);
    try {
      fs::remove(fpath);
    }
    catch (const exception& e) {
      spdlog::warn("Unable to remove temporary file fpath={}: {}", fpath, e.what());
    }
    zonesCreated.clear();
    zonesRemoved.clear();
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

  if (d_apiClient == nullptr) {
    // TODO actually send an error?
    return SR_ERR_OK;
  }

  pdns_api::ZonesApi zoneApiClient(d_apiClient);
  try {
    // We use a blocking call
    auto res = zoneApiClient.listZones("localhost", boost::none);
    auto zones = res.get();
    for (const auto &zone : zones) {
      libyang::S_Data_Node zoneNode(new libyang::Data_Node(parent, mod, "zones"));
      libyang::S_Data_Node classNode(new libyang::Data_Node(zoneNode, mod, "class", "IN"));
      libyang::S_Data_Node nameNode(new libyang::Data_Node(zoneNode, mod, "name", zone->getName().c_str()));
      libyang::S_Data_Node serialNode(new libyang::Data_Node(zoneNode, mod, "serial", to_string(zone->getSerial()).c_str()));

      // Lowercase zonekind
      string zoneKind = zone->getKind();
      std::transform(zoneKind.begin(), zoneKind.end(), zoneKind.begin(), [](unsigned char c){ return std::tolower(c); });
      libyang::S_Data_Node zonetypeNode(new libyang::Data_Node(zoneNode, mod, "zonetype", zoneKind.c_str()));
    }
  } catch (const web::uri_exception &e) {
    spdlog::warn("Unable to retrieve zones from from server: {}", e.what());
  } catch (const web::http::http_exception &e) {
    spdlog::warn("Unable to retrieve zones from from server: {}", e.what());
  } catch (const std::runtime_error &e) {
    spdlog::warn("Could not create zonestatus: {}", e.what());
  }

  return SR_ERR_OK;
}
} // namespace pdns_conf