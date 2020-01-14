/**
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

#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend {
  nlohmann::json::array_t RemoteBackend::getRecordsFromRRSetNode(const libyang::S_Data_Node &node) {
    auto ret = nlohmann::json::array();
    auto nodeSchemaPath = node->schema()->path();
    if (nodeSchemaPath != "/pdns-server:zones/pdns-server:zones/pdns-server:rrset") {
        throw std::range_error(fmt::format("Node {} is not /pdns-server:zones/pdns-server:zones/pdns-server:rrset", nodeSchemaPath));
    }
    // childNode is rrset[owner][type]/owner
    auto childNode = node->child();

    auto record = nlohmann::json();
    /* Iterate over childNode and its siblings (owner, ttl, type, rdata).
     * As rdata is the last node, we can rely on record being filled properly when handling
     * the rdata.
     */
    for (auto const &rrsetNode : childNode->tree_for()) {
      if ((rrsetNode->schema()->nodetype() & LYS_CONTAINER) && string(rrsetNode->schema()->name()) == "rdata") {
        // rdataNode is rrset[owner][type]/rdata/<type container>
        auto rdataNode = rrsetNode->child();
        if (record["qtype"] == "SOA") {
          // rdataNode is now the 'rrset/rdata/SOA/mname' node
          rdataNode = rdataNode->child();
          string content;
          for (auto const& soaNode : rdataNode->tree_for()) {
            auto soaLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(soaNode);
            content += " ";
            content += soaLeaf->value_str();
          }
          record["content"] = content.substr(1);
          ret.push_back(record);
        } else {
          // rdataNode is now the 'rrset/rdata/<rrset type>/<whatever the node is called>' node
          rdataNode = rdataNode->child();
          if (rdataNode->schema()->nodetype() & LYS_LEAF) {
              auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(rdataNode);
              record["content"] = leaf->value_str();
              ret.push_back(record);
          } else if (rdataNode->schema()->nodetype() & LYS_LEAFLIST) {
            for (auto const &dataNode : rdataNode->tree_for()) {
              auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(dataNode);
              record["content"] = leaf->value_str();
              ret.push_back(record);
            }
          }
        }
        continue;
      }

      if (!(rrsetNode->schema()->nodetype() & (LYS_LEAFLIST | LYS_LEAF))) {
        continue;
      }
      auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(rrsetNode);
      string leafName(rrsetNode->schema()->name());
      if (leafName == "owner") {
        record["qname"] = leaf->value_str();
      }
      if (leafName  == "type") {
        record["qtype"] = leaf->value_str();
      }
      if (leafName  == "ttl") {
        record["ttl"] = leaf->value()->uintu32();
      }
    }
    return ret;
  }

  libyang::S_Data_Node RemoteBackend::getZoneTree(sysrepo::S_Session session) {
    if (!session) {
      session = getSession();
    }
    return session->get_subtree("/pdns-server:zones/zones");
  }

  sessionErrors RemoteBackend::getErrorsFromSession(const sysrepo::S_Session& session) {
    if (!session) {
      throw std::logic_error("Session is a nullptr");
    }

    sessionErrors ret;
    auto errors = session->get_error();
    for (size_t i = 0; i < errors->error_cnt(); i++) {
      string xpath = errors->xpath(i) == nullptr ? "" : errors->xpath(i);
      string message = errors->message(i) == nullptr ? "" : errors->message(i);
      ret.push_back(std::make_pair(xpath, message));
    }
    return ret;
  }
}