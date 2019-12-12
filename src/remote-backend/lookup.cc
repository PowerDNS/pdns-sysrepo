#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::lookup(const Rest::Request& request, Http::ResponseWriter response) {
  auto recordname = request.param(":recordname").as<std::string>();
  auto recordtype = request.param(":type").as<std::string>();
  /*
  if (recordname == "example.com." && recordtype == "SOA") {
    nlohmann::json ret = {{
      "result", nlohmann::json::array({
        {{"qtype", "SOA"}, {"qname", "example.com."}, {"content", "a.b ns1.example.net. 1 2 3 4 5"}, {"ttl", 1200}}
      })
    }};
    response.send(Http::Code::Ok, ret.dump());
  }
  */
  string zoneName;
  try {
    zoneName = findBestZone(recordname);
  } catch (const std::out_of_range &e) {
    sendError(response, e.what());
    return;
  }
  if (zoneName.empty()) {
    sendError(response, "No zone for qname");
  }
  // TODO actually look for the right records
}
} // namespace pdns_sysrepo::remote_backend