/**
 * Copyright Pieter Lexis <pieter.lexis@powerdns.com>
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
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace pdns_sysrepo::remote_backend {
  nlohmann::json::array_t RemoteBackend::getRecordsFromRRSetNode(const libyang::S_Data_Node &node, const string &rrsetLocation) {
    auto ret = nlohmann::json::array();
    auto nodeSchemaPath = node->schema()->path();
    if (nodeSchemaPath != fmt::format("/pdns-server:zones/pdns-server:zones/pdns-server:{}", rrsetLocation)) {
        throw std::range_error(fmt::format("Node {} is not /pdns-server:zones/pdns-server:zones/pdns-server:{}", nodeSchemaPath, rrsetLocation));
    }

    // childNode is rrset[owner][type]/any_of(owner,rdata,ttl,....)
    auto childNode = node->child();

    string qname, qtype;
    uint32_t ttl;
    nlohmann::json record;
    for (auto const &rrsetNode : childNode->tree_for()) {
      if ((rrsetNode->schema()->nodetype() & LYS_CONTAINER) && string(rrsetNode->schema()->name()) == "rdata") {
        // rdataNode is rrset[owner][type]/rdata/<type container>
        auto rdataNode = rrsetNode->child();
        std::vector<string> parts;
        std::string path(rdataNode->path());
        boost::split(parts, path, boost::is_any_of("/"));
        if (parts.back() == "SOA") {
          // rdataNode is now the 'rrset/rdata/<TYPE>/<SOME_NODE>' node or one of its siblings
          rdataNode = rdataNode->child();
          std::map<string, string> recordElements;
          for (auto const& soaNode : rdataNode->tree_for()) {
            auto rdataLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(soaNode);
            string leafName = rdataLeaf->schema()->name();
            recordElements[leafName] = rdataLeaf->value_str();
          }
          record["content"] = fmt::format("{} {} {} {} {} {} {}", recordElements["mname"], recordElements["rname"], recordElements["serial"], recordElements["refresh"], recordElements["retry"], recordElements["expire"], recordElements["minimum"]);
          ret.push_back(record);
        } else if (parts.back() == "MX") {
          bool havePref = false;
          uint16_t pref;
          bool haveExchange = false;
          string exchange;
          for (auto const &n : rdataNode->tree_dfs()) {
            string name(n->schema()->name());
            if (name == "preference") {
              havePref = true;
              auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(n);
              pref = leaf->value()->uint16();
            } else if (name == "exchange") {
              haveExchange = true;
              auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(n);
              exchange = leaf->value_str();
            }
            if (haveExchange && havePref) {
              havePref = false;
              haveExchange = false;
              record["content"] = fmt::format("{} {}", pref, exchange);
              ret.push_back(record);
            }
          }
        } else {
          // This is for all records with one leaf or leaf-list like A, AAAA and CNAME
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
      }

      if (!(rrsetNode->schema()->nodetype() & (LYS_LEAFLIST | LYS_LEAF))) {
        continue;
      }
      auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(rrsetNode);
      string leafName(rrsetNode->schema()->name());
      if (leafName == "owner") {
        record["qname"] = leaf->value_str();
        qname = leaf->value_str();
      }
      if (leafName  == "type") {
        record["qtype"] = leaf->value_str();
        qtype = leaf->value_str();
      }
      if (leafName  == "ttl") {
        record["ttl"] = leaf->value()->uint32();
        ttl = leaf->value()->uint32();
      }
    }
    // Set the qname, qtype and TTL explicitly in case the nodes were not in the YANG order
    for (auto &r: ret) {
      r["qname"] = qname;
      r["qtype"] = qtype;
      r["ttl"] = ttl;
    }
    return ret;
  }

  libyang::S_Data_Node RemoteBackend::getZoneTree(sysrepo::S_Session session) {
    static const string zonesRootPath = "/pdns-server:zones";
    if (!session) {
      session = getSession();
    }
    auto ret = session->get_subtree(zonesRootPath.c_str());
    if (!ret) {
      throw std::range_error(fmt::format("{} not found in sysrepo", zonesRootPath));
    }
    return ret;
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

  void RemoteBackend::logSessionErrors(const sysrepo::S_Session& session, const spdlog::level::level_enum &level) {
    auto errors = getErrorsFromSession(session);
    for (auto const& error : errors) {
      spdlog::log(level, "  Error xpath='{}' message='{}'", error.first, error.second);
    }
  }
}
