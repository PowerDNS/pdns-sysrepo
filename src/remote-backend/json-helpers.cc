/**
 * Copyright 2019-2020 Pieter Lexis <pieter.lexis@powerdns.com>
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
nlohmann::json RemoteBackend::makeDomainInfo(const std::string& zone, const std::string& kind) {
    nlohmann::json ret;
    ret["id"] = getDomainID(zone);
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
