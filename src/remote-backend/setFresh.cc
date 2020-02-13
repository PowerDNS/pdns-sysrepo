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
void RemoteBackend::setFresh(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
    logRequest(request);
    uint32_t zoneId = request.param(":id").as<uint32_t>();
    d_slaveFreshnessChecks[zoneId] = time(nullptr);
    sendResponse(request, response, nlohmann::json({{"result", true}}));
}
}