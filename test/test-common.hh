#pragma once

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
