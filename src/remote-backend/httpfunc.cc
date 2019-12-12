#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::sendError(Http::ResponseWriter& response, const string& error, const Http::Code& code) {
  nlohmann::json ret = {{
    "result", false
  }};
  if (!error.empty()) {
      ret["log"] = nlohmann::json::array({
          error
      });
  }
  response.setMime(s_application_json_mediatype);
  response.send(code, ret.dump());
}

void RemoteBackend::sendResponse(Http::ResponseWriter& response, const nlohmann::json &ret) {
  response.setMime(s_application_json_mediatype);
  response.send(Http::Code::Ok, ret.dump());
}

void RemoteBackend::notFound(const Rest::Request &request, Http::ResponseWriter response) {
  sendError(response, string(), Http::Code::Not_Found);
}

} // namespace pdns_sysrepo::remote_backend