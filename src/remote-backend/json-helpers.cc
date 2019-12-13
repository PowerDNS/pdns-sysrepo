#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
nlohmann::json RemoteBackend::makeDomainInfo(const std::string& zone, const std::string& kind) {
    nlohmann::json ret;
    ret["id"] = -1;
    ret["zone"] = zone;
    ret["kind"] = kind;
    ret["serial"] = 0;
    return ret;
}
} // namespace pdns_sysrepo::remote_backend
