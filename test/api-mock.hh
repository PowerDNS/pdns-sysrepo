/*
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
#pragma once

#include "test-common.hh"

#include <string>
#include <cpprest/json.h>
#include "httpmockserver/mock_server.h"
#include "httpmockserver/test_environment.h"

using std::string;

class PDNSApiMock : public httpmock::MockServer
{
public:
  /// Create HTTP server on port 9200
  explicit PDNSApiMock(int port = 9200) :
    MockServer(port){}

private:
  /// Handler called by MockServer on HTTP request.
  Response responseHandler(
    const std::string& url,
    const std::string& method,
    const std::string& data,
    const std::vector<UrlArg>& urlArguments,
    const std::vector<Header>& headers) {

    auto r = Response();
    r.addHeader(httpmock::MockServer::Header("Content-Type", "application/json"));

    for (auto const& h : headers) {
      if (h.key == "X-API-Key" && h.value != APIKEY) {
        r.status = 401;
        r.body = "{\"error\": \"not authorized\"";
        return r;
      }
    }

    // https://github.com/Microsoft/cpprestsdk/wiki/JSON
    web::json::value jsonData;
    if (!data.empty()) {
      jsonData = web::json::value::parse(data);
    }

    if (method == "GET" && matchesPrefix(url, "/api/v1/servers/localhost/zones")) {
      for (auto const& a : urlArguments) {
        if (a.key == "zone" && a.value == "example.com.") {
          r.status = 200;
          r.body = "[{\"account\": \"\", \"dnssec\": false, \"edited_serial\": 2019112701, \"id\": \"example.com.\", \"kind\": \"Native\", \"last_check\": 0, \"masters\": [], \"name\": \"example.com.\", \"notified_serial\": 0, \"serial\": 2019112701, \"url\": \"/api/v1/servers/localhost/zones/example.com.\"}]";
          return r;
        }
      }
    }

    if (method == "POST" && matchesPrefix(url, "/api/v1/servers/localhost/zones")) {
      cout<<jsonData["kind"].as_string()<<endl;
      cout<<jsonData["name"].as_string()<<endl;
      if (jsonData["kind"].as_string() == "master" && jsonData["name"].as_string() == "create.example.com.") {

        jsonData["id"] = web::json::value("create.example.com.");
        jsonData["dnssec"] = web::json::value(false);
        jsonData["serial"] = web::json::value(0);
        jsonData["masters"] = web::json::value::array();

        utility::stringstream_t stream;
        jsonData.serialize(stream);
        r.body = stream.str();
        r.status = web::http::status_codes::Created;
        return r;
      }
    }

    if (method == "PUT" && matchesPrefix(url, "/api/v1/servers/localhost/zones/example.com.")) {
      if (jsonData["kind"].as_string() == "master") {
        return Response(204, "No Data");
      }
      return Response(422, "Wrong data");
    }
    // Return "URI not found" for the undefined methods
    return Response(404, "Not Found");
  }

  /// Return true if \p url starts with \p str.
  bool matchesPrefix(const std::string& url, const std::string& str) const {
    return url.substr(0, str.size()) == str;
  }
};
