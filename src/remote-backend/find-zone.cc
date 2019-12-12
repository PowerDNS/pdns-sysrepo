#include <boost/algorithm/string/join.hpp>
#include <fmt/format.h>

#include "remote-backend.hh"
#include <spdlog/spdlog.h>

namespace pdns_sysrepo::remote_backend {

using std::vector;

vector<string> split(const string& str, const string& delim) {
  vector<string> tokens;
  size_t prev = 0, pos = 0;
  do {
    pos = str.find(delim, prev);
    if (pos == string::npos)
      pos = str.length();
    string token = str.substr(prev, pos - prev);
    if (!token.empty())
      tokens.push_back(token);
    prev = pos + delim.length();
  } while (pos < str.length() && prev < str.length());
  return tokens;
}

string RemoteBackend::findBestZone(const string& qname) {
  // I miss dnsname.cc :(
  auto labels = split(qname, ".");
  string ret = qname;

  auto tree = d_session->get_subtree("/pdns-server:zones");

  // haha, no root support
  while (!labels.empty()) {
    if (tree->find_path(fmt::format("/pdns-server:zones[name='{}']", ret).c_str())->number() > 0) {
      return ret;
    }
    labels.erase(labels.begin());
    ret = boost::algorithm::join(labels, ".") + ".";
  }
  throw std::out_of_range(fmt::format("No zone found for qname {}", qname));
}
}