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

nlohmann::json RemoteBackend::makeRecord(const std::string& name, const std::string& rtype, const std::string& content, const uint16_t ttl) {
    nlohmann::json ret;
    ret["name"] = name;
    ret["type"] = rtype;
    ret["content"] = content;
    ret["ttl"] = ttl;
    return ret;

}
} // namespace pdns_sysrepo::remote_backend
