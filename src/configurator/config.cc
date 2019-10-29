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

#include <string>

#include <yaml-cpp/yaml.h>

#include "config.hh"

using namespace std;

namespace pdns_conf
{
Config::Config(string fpath) :
  d_config_path(fpath) {
  YAML::Node c;
  try {
    c = YAML::LoadFile(fpath);
  }
  catch (const YAML::BadFile& bf) {
    throw runtime_error("Unable to load configuration '" + fpath + "': " + bf.what());
  }
  if (c["pdns_conf"]) {
    d_pdns_conf = c["pdns_conf"].as<string>();
  }
}
} // namespace pdns_conf