#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::setNotified(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
  logRequest(request);
  uint32_t zoneId = request.param(":id").as<uint32_t>();

  // Pistache has no parser for form data......
  string body = request.body();
  string decodedBody = urlDecode(body);
  auto pos = decodedBody.find("=");
  auto serialStr = decodedBody.substr(pos + 1);
  uint32_t serial = std::stoi(serialStr);

  d_notifiedMasters[zoneId] = serial;

  sendResponse(request, response, nlohmann::json({{"result", true}}));
}
} // namespace pdns_sysrepo::remote_backend