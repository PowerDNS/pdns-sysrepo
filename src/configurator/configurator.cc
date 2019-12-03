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

#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>

#include <boost/filesystem.hpp>
#include <mstch/mstch.hpp>
#include <spdlog/spdlog.h>

#include "configurator.hh"
#include "util.hh"

using std::make_shared;
using std::range_error;
using std::runtime_error;
namespace fs = boost::filesystem;

namespace pdns_conf
{
static const string pdns_conf_template = R"(
local-address =
{{ #local-address }}
local-address += {{ address }} # {{ name }}
{{ /local-address }}
master = {{ master }}
slave = {{ slave }}

launch =
{{ #launch }}
launch += {{ backendtype }}:{{ name }}
{{ /launch }}
{{ # backend-options }}
{{ backendtype }}-{{name}}-{{ option }} = {{ value }}
{{ /backend-options }}

webserver = {{ webserver }}
webserver-address = {{ webserver-address }}
webserver-max-bodysize = {{ webserver-max-body-size }}
webserver-loglevel = {{ webserver-loglevel }}
webserver-allow-from =
{{ #webserver-allow-from }}
webserver-allow-from += {{ . }}
{{ /webserver-allow-from }}
api = {{ api }}
api-key = {{ api-key }}
)";

PdnsServerConfig::PdnsServerConfig(const libyang::S_Data_Node &node) {
  // spdlog::debug("root node name={} schema_type={} path={}", node->schema()->name(), libyangNodeType2String(node->schema()->nodetype()), node->schema()->path());

  libyang::S_Data_Node_Leaf_List leaf;
  string nodename(node->schema()->name());

  if (node->schema()->nodetype() == LYS_CONTAINER && nodename == "pdns-server") {
    auto child = node->child();
    if (child == nullptr) {
      // We were passed an empty container, use all the defaults
      return;
    }
    /*
     * Assume the tree looks like this:
     *                /- pdns-server ---------------\
     *               /     |                         \
     *            master  listen-address[name=foo]    listen-address[name=bar]
     *                       /          |      \         |       |           \
     *                     name  ip-address    port     name    ip-address   port
     * 
     * We walk through the tree horizontally, skipping the root. Each child need its own
     * parsing code for its own data, as the it can be a leaf-list, but the top-level loop
     * will **only** go through all siblings below pdns-server.
     * 
     * In this example, we first see 'master' and handle it.
     * Next we see 'listen-address[name=foo]' and we know it is a leaf-list. We use a depth-first search
     * (even though we know listen-address is one level deep) to parse all the leafs into a
     * struct and adding that struct into a vector.
     * 
     * Then we see 'listen-address[name=bar]' and do the same thing.
     */
    do {
      nodename = child->schema()->name();
      // spdlog::debug("node name={} schema_type={} path={}", child->schema()->name(), libyangNodeType2String(child->schema()->nodetype()), child->schema()->path());
      if (child->schema()->nodetype() == LYS_LEAF) {
        leaf = make_shared<libyang::Data_Node_Leaf_List>(child);
      }
      if(nodename == "master") {
        master = leaf->value()->bln();
      }
      if (nodename == "slave") {
        slave = leaf->value()->bln();
      }
      if (nodename == "listen-addresses") {
        listenAddress la;
        for (const auto &n: child->tree_dfs()) {
          if(n->schema()->nodetype() == LYS_LEAF) {
            string leafName(n->schema()->name());
            leaf = make_shared<libyang::Data_Node_Leaf_List>(n);
            if(leafName == "name") {
              la.name = leaf->value()->string();
            }
            if(leafName == "ip-address") {
              la.address = ComboAddress(leaf->value()->string(), la.address.getPort());
            }
            if(leafName == "port") {
              la.address.setPort(leaf->value()->uint16());
            }
          }
        }
        listenAddresses.push_back(la);
      }

      if (nodename == "backend") {
        backend b;
        for (const auto &n: child->tree_dfs()) {
          if(n->schema()->nodetype() == LYS_LEAF) {
            string leafName(n->schema()->name());
            leaf = make_shared<libyang::Data_Node_Leaf_List>(n);
            if(leafName == "name") {
              b.name = leaf->value_str();
            }
            else if(leafName == "backendtype") {
              b.backendtype = leaf->value_str();
            }
            else {
              b.options.push_back({
                leafName,
                leaf->value_str()
              });
            }
          }
        }
        backends.push_back(b);
      }

      if (nodename == "webserver") {
        for (const auto& n : child->tree_dfs()) {
          if (n->schema()->nodetype() == LYS_CONTAINER) {
            // The first child is the container leaf
            continue;
          }
          leaf = make_shared<libyang::Data_Node_Leaf_List>(n);
          string leafName(n->schema()->name());
          if (leafName == "password") {
            webserver.password = leaf->value_str();
            webserver.webserver = true;
          }
          if (leafName == "api-key") {
            webserver.api_key = leaf->value_str();
            webserver.api = true;
          }
          if (leafName == "address") {
            webserver.address = ComboAddress(leaf->value_str(), webserver.address.getPort());
          }
          if (leafName == "port") {
            webserver.address.setPort(leaf->value()->uint16());
          }
          if (leafName == "allow-from") {
            webserver.allow_from.push_back(leaf->value_str());
          }
          if (leafName == "loglevel") {
            webserver.loglevel = leaf->value_str();
          }
          if (leafName == "max-body-size") {
            webserver.max_body_size = leaf->value()->uintu32();
          }
        }
      }
    } while(child = child->next());
  }
}

void PdnsServerConfig::writeToFile(const string &fpath) {
  spdlog::debug("Attempting to create configuration file={}", fpath);
  auto p = fs::path(fpath);
  auto d = p.remove_filename();
  if (!fs::is_directory(d)) {
    // TODO find better exception
    throw range_error(d.string() + " is not a directory");
  }

  std::ofstream outputFile(fpath);
  if (!outputFile) {
    throw runtime_error("Unable to open output file '" + fpath + "'");
  }

  outputFile << getConfig();
  outputFile.close();
  spdlog::trace("Written config file {}", fpath);
}

string PdnsServerConfig::getConfig() {
  // Don't do HTML escaping, we're not a webserver
  mstch::config::escape = [](const std::string& str) -> std::string {
    return str;
  };

  mstch::array laddrs;
  for (const auto& la : listenAddresses) {
    laddrs.push_back(mstch::map{
      {"name", la.name},
      {"address", la.address.toStringWithPort()}});
  }

  mstch::array backendOptions;
  mstch::array backendLaunch;
  for (const auto& b : backends) {
      backendLaunch.push_back(mstch::map{
        {"name", b.name},
        {"backendtype", b.backendtype}});

    for (const auto& o : b.options) {
      backendOptions.push_back(mstch::map{
        {"name", b.name},
        {"option", o.first},
        {"value", o.second},
        {"backendtype", b.backendtype}
      });
    }
  }

  // :(
  mstch::array webserverAllowFrom;
  for (const auto& a : webserver.allow_from) {
    webserverAllowFrom.push_back(a);
  }

  mstch::map ctx{
    {"master", bool2str(master)},
    {"slave", bool2str(slave)},
    {"local-address", laddrs},
    {"launch", backendLaunch},
    {"backend-options", backendOptions},
    {"webserver", bool2str(webserver.webserver)},
    {"webserver-address", webserver.address.toStringWithPort()},
    {"webserver-max-body-size", std::to_string(webserver.max_body_size)},
    {"webserver-loglevel", webserver.loglevel},
    {"webserver-allow-from", webserverAllowFrom},
    {"api", bool2str(webserver.api)},
    {"api-key", webserver.api_key}
  };

  spdlog::trace("Generated config:\n{}", mstch::render(pdns_conf_template, ctx));

  return mstch::render(pdns_conf_template, ctx);
}

string PdnsServerConfig::bool2str(const bool b) {
  return b ? "yes" : "no";
}
} // namespace pdns_conf