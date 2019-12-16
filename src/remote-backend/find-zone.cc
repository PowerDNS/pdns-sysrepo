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
  auto session = getSession();

  // haha, no root support
  while (!labels.empty()) {
    try {
      string xpath = fmt::format("/pdns-server:zones/zones[name='{}']", ret);
      spdlog::trace("Attempting to find zone '{}' xpath={}", ret, xpath);
      auto data = session->get_items(xpath.c_str());
      if (data->val_cnt() > 0) {
        spdlog::trace("Found zone '{}'", ret);
        return ret;
      }
      spdlog::trace("not found");
    }
    catch (const sysrepo::sysrepo_exception& e) {
      // Do nothing
    }
    labels.erase(labels.begin());
    ret = boost::algorithm::join(labels, ".") + ".";
  }
  throw std::out_of_range(fmt::format("No zone found for qname {}", qname));
}
}