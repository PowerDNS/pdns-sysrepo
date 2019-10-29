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