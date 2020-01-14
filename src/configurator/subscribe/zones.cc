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
#include <boost/algorithm/string/join.hpp>

#include "../subscribe.hh"
#include "../util.hh"
#include "api/ZonesApi.h"
#include "api/ServersApi.h"
#include "model/Zone.h"

namespace pdns_model = org::openapitools::client::model;

namespace pdns_conf
{
void ServerConfigCB::changeZoneAddAndDelete(sysrepo::S_Session& session) {
  // This fetches only full zone nodes, not subnodes.
  auto iter = session->get_changes_iter("/pdns-server:zones/pdns-server:zones");
  auto change = session->get_change_tree_next(iter);

  if (change == nullptr) {
    return;
  }

  // Sanity check, if this throws, we can't do anything...
  {
    auto sApi = pdns_api::ServersApi(d_apiClient);
    sApi.listServers().get();
  }
  
  auto sess = static_pointer_cast<sr::Session>(session);

  while (change != nullptr && change->node() != nullptr) {
    spdlog::trace("zones change. operation={}", util::srChangeOper2String(change->oper()));

    if (change->oper() == SR_OP_CREATED) {
      auto child = change->node()->child();
      pdns_api::Zone z;
      string zoneName;
      while (child) {
        auto leaf = make_shared<libyang::Data_Node_Leaf_List>(child);
        string leafName = leaf->schema()->name();
        // spdlog::trace("child name={} value={}", leaf->schema()->name(), leaf->value_str());
        if (leafName == "name") {
          zoneName = leaf->value_str();
          z.setName(leaf->value_str());
        }
        if (leafName == "zonetype") {
          z.setKind(leaf->value_str());
        }
        if (leafName == "masters") {
          auto masters = z.getMasters();
          masters.push_back(leaf->value_str());
          z.setMasters(masters);
        }
        if (leafName == "class" && !(string(leaf->value_str()) != "IN" || string(leaf->value_str()) != "1")) {
          throw std::runtime_error(fmt::format("Zone {} can't be validated, class is not IN but {}", z.getName(), leaf->value_str()));
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
    change = session->get_change_tree_next(iter);
  }
}

void ServerConfigCB::changeZoneModify(sysrepo::S_Session &session) {
  // Fetch the sub-nodes of all zones that changed
  auto iter = session->get_changes_iter("/pdns-server:zones/pdns-server:zones/*");
  auto change = session->get_change_tree_next(iter);

  pdns_api::ZonesApi zoneApiClient(d_apiClient);

  while (change != nullptr && change->node() != nullptr) {
    // spdlog::trace("Zone modify. operation={} path={}, node_path={}, list_pos={}", util::srChangeOper2String(change->oper()), change->node()->path(), change->node()->schema()->path(), change->node()->list_pos());
    auto namenode = change->node()->parent()->find_path("/pdns-server:zones/pdns-server:zones/pdns-server:name")->data().at(0);
    if (!namenode) {
      throw std::runtime_error("Unable to find the name of a changed zone!");
    }
    string zoneName = make_shared<libyang::Data_Node_Leaf_List>(namenode)->value_str();
    if (std::find(zonesRemoved.begin(), zonesRemoved.end(), zoneName) != zonesRemoved.end()) {
      // No need to do things to a zone that is removed
      change = session->get_change_tree_next(iter);
      continue;
    }
    pdns_api_model::Zone z = zonesModified[zoneName];
    auto leaf = make_shared<libyang::Data_Node_Leaf_List>(change->node());
    string leafName = leaf->schema()->name();

    if (change->oper() == SR_OP_MODIFIED) {
      if (leafName == "zonetype") {
        z.setKind(leaf->value_str());
      }
    }
    else if(change->oper() == SR_OP_CREATED || change->oper() == SR_OP_DELETED) {
      if (leafName == "masters") {
        // No smart tricks, the current session already has the new values
        auto masters = static_pointer_cast<sr::Session>(session)->getZoneMasters(zoneName);
        z.setMasters(masters);
      }
    }

    zonesModified[zoneName] = z;
    change = session->get_change_tree_next(iter);
  }
}

void ServerConfigCB::doneZoneAddAndDelete() {
  vector<string> errors;
  pdns_api::ZonesApi zonesApiClient(d_apiClient);

  for (auto const& z : zonesCreated) {
    string err = fmt::format("Unable to create zone '{}'", z.getName());
    try {
      auto zp = make_shared<pdns_api_model::Zone>(z);
      spdlog::debug("Creating zone {}", zp->getName());
      auto zone = zonesApiClient.createZone("localhost", zp, false).get();
      spdlog::debug("Created zone {}", zp->getName());
    }
    catch (const pdns_api::ApiException& e) {
      errors.push_back(fmt::format("{}: {}", err, e.what()));
      spdlog::warn(err);
    }
    catch (const std::exception& e) {
      errors.push_back(fmt::format("{}: {}", err, e.what()));
      spdlog::warn(err);
    }
  }

  for (auto const& z : zonesRemoved) {
    string err = fmt::format("Unable to delete zone '{}'", z);
    try {
      string zoneId = getZoneId(z);
      spdlog::debug("Deleting zone {}", z);
      zonesApiClient.deleteZone("localhost", zoneId).get();
      spdlog::debug("Deleted zone {}", z);
    }
    catch (const pdns_api::ApiException& e) {
      errors.push_back(fmt::format("{}: {}", err, e.what()));
      spdlog::warn(err);
    }
    catch (const std::exception& e) {
      errors.push_back(fmt::format("{}: {}", err, e.what()));
      spdlog::warn(err);
    }
  }
  // TODO Store zones that could not be created or deleted and try again later, returning SR_ERR_CALLBACK_SHELVE and starting a thread to try the creation again
  zonesCreated.clear();
  zonesRemoved.clear();
  if (!errors.empty()) {
    throw std::runtime_error(boost::algorithm::join(errors, ", "));
  }
}

void ServerConfigCB::doneZoneModify() {
  vector<string> errors;
  for (auto const &z : zonesModified) {
    auto zoneName = z.first;
    auto zone = z.second;
    string zoneId;
    try {
      zoneId = getZoneId(zoneName);
    } catch (const runtime_error &e) {
      errors.push_back(fmt::format("unable to modify zone {}: {}", zoneName, e.what()));
      continue;
    }

    pdns_api::ZonesApi zonesApiClient(d_apiClient);
    auto zp = make_shared<pdns_api_model::Zone>(zone);
    try {
      zonesApiClient.putZone("localhost", zoneId, zp).get();
    }
    catch (const pdns_api::ApiException& e) {
      errors.push_back(fmt::format("Unable to modify zone {}: {}", zoneId, e.what()));
      spdlog::debug("API PUT call to modify zone {} returned: {}", zoneId, e.getContent()->get());
      continue;
    }
  }
  zonesModified.clear();
  if (!errors.empty()) {
    throw runtime_error(boost::algorithm::join(errors, ", "));
  }
}
} // namespace pdns_conf