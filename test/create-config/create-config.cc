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
#include <stdio.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <libyang/Libyang.hpp>

#include "configurator/configurator.hh"

using std::make_shared;
using std::runtime_error;
using std::cerr;
using std::endl;

using ::testing::HasSubstr;
using ::testing::MatchesRegex;

#define MODULE_NAME "pdns-server"

string yangDir;

libyang::S_Data_Node getBasicConfig() {
  libyang::S_Context ctx = make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server:pdns-server", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  return node;
}

libyang::S_Data_Node getMasterConfig() {
  libyang::S_Context ctx = make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server:pdns-server", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  libyang::S_Data_Node master(new libyang::Data_Node(node, mod, "master", "true"));
  return node;
}

void testAllBasicsExcept(const string &config, const string &e) {
  if (e != "master") {
    ASSERT_THAT(config, HasSubstr("master = no\n"));
  }
  if (e != "slave") {
    ASSERT_THAT(config, HasSubstr("slave = no\n"));
  }
  if (e != "launch") {
    ASSERT_THAT(config, HasSubstr("launch =\n"));
  }
  if (e != "local-address") {
  ASSERT_THAT(config, HasSubstr("local-address =\n"));
  }
  if (e != "webserver") {
    ASSERT_THAT(config, HasSubstr("webserver = no\n"));
  }
  if (e != "webserver-loglevel") {
    ASSERT_THAT(config, HasSubstr("webserver-loglevel = normal\n"));
  }
}

TEST(config_test, empty_config) {
  auto node = getBasicConfig();
  auto configurator = pdns_conf::PdnsServerConfig(node);
  auto config = configurator.getConfig();
  testAllBasicsExcept(config, "none");
}

TEST(config_test, master) {
  auto node = getMasterConfig();
  auto configurator = pdns_conf::PdnsServerConfig(node);
  auto config = configurator.getConfig();
  testAllBasicsExcept(config, "master");
  ASSERT_THAT(config, HasSubstr("master = yes\n"));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  if (argc != 2) {
    cerr<<"Please prove the path to the yang directory"<<endl;
    return 1;
  }
  yangDir = argv[1];
  return RUN_ALL_TESTS();
}