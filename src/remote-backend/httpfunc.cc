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

std::string RemoteBackend::urlDecode(std::string& eString) {
  // from http://www.cplusplus.com/forum/general/94849/
  std::string ret;
  char ch;
  size_t i;
  unsigned int j;
  for (i = 0; i < eString.length(); i++) {
    if (int(eString[i]) == 37) {
      sscanf(eString.substr(i + 1, 2).c_str(), "%x", &j);
      ch = static_cast<char>(j);
      ret += ch;
      i = i + 2;
    }
    else {
      ret += eString[i];
    }
  }
  return (ret);
}

} // namespace pdns_sysrepo::remote_backend