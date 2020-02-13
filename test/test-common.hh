/*
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
#pragma once

#include <stdio.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>

#define APIKEY "foo"
#define MODULE_NAME "pdns-server"
std::string yangDir;

libyang::S_Data_Node getBasicConfig() {
  libyang::S_Context ctx = std::make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = std::make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server:pdns-server", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  return node;
}

libyang::S_Data_Node getMasterConfig() {
  libyang::S_Context ctx = std::make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = std::make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server:pdns-server", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  libyang::S_Data_Node master(new libyang::Data_Node(node, mod, "master", "true"));
  return node;
}
