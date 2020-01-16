/**
 * Copyright Pieter Lexis <pieter.lexis@powerdns.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::sendError(const Rest::Request& request, Http::ResponseWriter& response, const string& error, const Http::Code& code) {
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
  logRequestResponse(request, response, ret);
}

void RemoteBackend::sendResponse(const Rest::Request& request, Http::ResponseWriter& response, const nlohmann::json &ret) {
  response.setMime(s_application_json_mediatype);
  response.send(Http::Code::Ok, ret.dump());
  logRequestResponse(request, response, ret);
}

void RemoteBackend::notFound(const Rest::Request &request, Http::ResponseWriter response) {
  sendError(request, response, string(), Http::Code::Not_Found);
}

std::string RemoteBackend::urlDecode(std::string& eString) {
  // from http://www.cplusplus.com/forum/general/94849/
  std::string ret;
  ret.reserve(eString.length());
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