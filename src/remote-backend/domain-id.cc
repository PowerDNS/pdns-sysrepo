#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
uint32_t getNewId(const boost::bimap<std::string, uint32_t> &map) {
  if (map.empty()) {
    return 1;
  }
  uint32_t highest = map.left.rbegin()->second;
  return highest++;
}

uint32_t RemoteBackend::getDomainID(const std::string& domain) {
  auto it = d_domainIds.left.find(domain);
  if (it != d_domainIds.left.end()) {
      return it->second;
  }
  auto id = getNewId(d_domainIds);
  d_domainIds.insert({domain, id});
  return id;
}

std::string RemoteBackend::getDomainFromId(const uint32_t id) {
  auto it = d_domainIds.right.find(id);
  if (it != d_domainIds.right.end()) {
      return it->second;
  }
  throw std::out_of_range(fmt::format("No domain known with id {}", id));
}

} // namespace pdns_sysrepo::remote_backend