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
#include <stdio.h>
#include <gtest/gtest.h>

#include <libyang/Libyang.hpp>
#include <libyang/Tree_Data.hpp>

#define MODULE_NAME "pdns-server-typedef-test"

using std::cerr;
using std::endl;
using std::string;
using std::make_shared;

string yangDir;

TEST(yang_typedef_test, valid_hostname) {
  libyang::S_Context ctx = make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server-typedef-test:hostnames", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  ASSERT_NO_THROW(libyang::S_Data_Node valid(new libyang::Data_Node(node, mod, "hostname", "my.hostname.example.")));
  ASSERT_NO_THROW(libyang::S_Data_Node valid(new libyang::Data_Node(node, mod, "hostname", "another-bla.hostname.example.")));
  ASSERT_NO_THROW(libyang::S_Data_Node valid(new libyang::Data_Node(node, mod, "hostname", "valid.example.")));
}

TEST(yang_typedef_test, invalid_hostname) {
  libyang::S_Context ctx = make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server-typedef-test:hostnames", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  ASSERT_THROW(libyang::S_Data_Node invalid(new libyang::Data_Node(node, mod, "hostname", "_invalid.hostname.example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node invalid(new libyang::Data_Node(node, mod, "hostname", "invalid-.hostname.example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node invalid(new libyang::Data_Node(node, mod, "hostname", "invalid.")), std::runtime_error);
}

TEST(yang_typedef_test, owner_name) {
  libyang::S_Context ctx = make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server-typedef-test:ownernames", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", ".")));

  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "example.")));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "foo-example.")));

  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "*.example.")));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "*.foo-example.")));

  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "foo.bar.example.")));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "foo-foo.bar.example.")));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "foo-foo.bar.example-foo.")));

  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "*.foo.bar.example.")));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "*.foo-foo.bar.example.")));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "*.foo.bar.example-foo.")));

  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "example--foo.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "foo--bar.example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "--bar.example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "bar--.example.")), std::runtime_error);

  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "*a.example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "-.example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "-foo.example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "foo-.example.")), std::runtime_error);

  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "foo..example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", ".example.")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "example..")), std::runtime_error);
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", "foo.example")), std::runtime_error);
}

TEST(yang_typedef_test, length) {
  libyang::S_Context ctx = make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server-typedef-test:ownernames", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  string too_long_label(64, 'a');
  string label_max_size(63, 'a');

  string s = label_max_size + ".";
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())));

  s = label_max_size + ".example.";
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())));

  s = "*." + label_max_size + ".";
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())));

  s = "*." + label_max_size + ".example.";
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())));

  s = too_long_label + ".";
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())), std::runtime_error);

  s = too_long_label + ".example.";
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())), std::runtime_error);
  
  s = too_long_label + "foo.example.";
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())), std::runtime_error);

  // 256
  s = label_max_size + "." + label_max_size + "." + label_max_size + "." + label_max_size + ".";
  ASSERT_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())), std::runtime_error);

  // 255
  s = string(62, 'a') + "." + label_max_size + "." + label_max_size + "." + label_max_size + ".";
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "owner-name", s.c_str())));
}

TEST(yang_typedef_test, zone_name) {
  libyang::S_Context ctx = make_shared<libyang::Context>(libyang::Context(yangDir.c_str()));
  auto mod = ctx->load_module(MODULE_NAME);

  libyang::S_Data_Node node = make_shared<libyang::Data_Node>(libyang::Data_Node(ctx, "/pdns-server-typedef-test:zonenames", nullptr, LYD_ANYDATA_CONSTSTRING, 0));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "zonename", ".")));
  ASSERT_NO_THROW(libyang::S_Data_Node n(new libyang::Data_Node(node, mod, "zonename", "example.")));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  if (argc != 2) {
    cerr<<"Please provide the path to the yang directory"<<endl;
    return 1;
  }
  yangDir = argv[1];
  return RUN_ALL_TESTS();
}