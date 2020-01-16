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

#include <boost/bimap.hpp>
#include <spdlog/spdlog.h>
#include <sysrepo-cpp/Session.hpp>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/net.h>
#include <nlohmann/json.hpp>

#pragma once

namespace pdns_sysrepo::remote_backend
{

/**
 * @brief type to shuttle syrepo session errors around
 * 
 * The pair is {xpath, error_message} 
 */
typedef std::vector<std::pair<std::string, std::string>> sessionErrors;

namespace Http = Pistache::Http;
namespace Rest = Pistache::Rest;
using std::string;

class RemoteBackend
{
public:
  RemoteBackend(sysrepo::S_Connection& connection, const Pistache::Address& addr);
  ~RemoteBackend();

  /**
   * @brief Starts the Remote Backend
   * 
   * Must be called for the backend to do something useful
   */
  void start();

protected:
  /**
   * @brief Implements the lookup endpoint
   * 
   * https://doc.powerdns.com/authoritative/backends/remote.html#lookup
   * 
   * @param request 
   * @param response 
   */
  void lookup(const Pistache::Rest::Request& request, Http::ResponseWriter response);

  /**
   * @brief Implements the getAllDomains endpoint
   * 
   * https://doc.powerdns.com/authoritative/backends/remote.html#getalldomains
   * 
   * @param request 
   * @param response 
   */
  void getAllDomains(const Pistache::Rest::Request& request, Http::ResponseWriter response);

  /**
   * @brief Implements the list endpoint
   * 
   * https://doc.powerdns.com/authoritative/backends/remote.html#list
   * 
   * @param request 
   * @param response 
   */
  void list(const Pistache::Rest::Request& request, Http::ResponseWriter response);

  /**
   * @brief Implements the getUpdatedMasters endpoint
   * 
   * https://doc.powerdns.com/authoritative/backends/remote.html#getupdatedmasters
   * 
   * @param request 
   * @param response 
   */
  void getUpdatedMasters(const Pistache::Rest::Request& request, Http::ResponseWriter response);

  /**
   * @brief Implements the setNotified endpoint
   * 
   * https://doc.powerdns.com/authoritative/backends/remote.html#setNotified
   * 
   * @param request 
   * @param response 
   */
  void setNotified(const Pistache::Rest::Request& request, Http::ResponseWriter response);

  /**
   * @brief Implements the getDomainMetadata endpoint
   * 
   * https://doc.powerdns.com/authoritative/backends/remote.html#getDomainMetadata
   * 
   * @param request 
   * @param response 
   */
  void getDomainMetadata(const Pistache::Rest::Request& request, Http::ResponseWriter response);

  /**
   * @brief Sends a 404 with {"result": false}
   * 
   * @param request 
   * @param response 
   */
  void notFound(const Pistache::Rest::Request& request, Http::ResponseWriter response);

  /**
   * @brief Finds the best zone for qname
   * 
   * @param qname 
   * @return string 
   */
  string findBestZone(const string &qname);

  /**
   * @brief Sends an error HTTP response
   * 
   * @param response 
   * @param error
   * @param code
   */
  void sendError(const Pistache::Rest::Request& request, Http::ResponseWriter& response, const std::string& error, const Http::Code &code = Http::Code::Internal_Server_Error);

  /**
   * @brief Send an HTTP 200 response with ret
   * 
   * @param response 
   * @param ret
   */
  void sendResponse(const Pistache::Rest::Request& request, Http::ResponseWriter& response, const nlohmann::json& ret);

  /**
   * @brief Returns a JSON object for DomainInfo
   * 
   * e.g:
   * { "id": -1,
   *   "zone":"unit.test.",
   *   "masters": ["10.0.0.1"],
   *   "serial":2,
   *   "kind":"native"}
   * 
   * All values should be provided by the caller
   * 
   * @param zone 
   * @param kind 
   * @return nlohmann::json 
   */
  nlohmann::json makeDomainInfo(const std::string &zone, const std::string &kind);


  /**
   * @brief Returns a JSON object for a record
   * 
   * @param name 
   * @param rtype 
   * @param content 
   * @param ttl 
   * @return nlohmann::json 
   */
  nlohmann::json makeRecord(const std::string& name, const std::string& rtype, const std::string& content, const uint16_t ttl);

  /**
   * @brief Retrieve an Array of records from an RRSet Node
   * 
   * Takes a node rooted at '/pdns-server:zones/zones[name]/rrset[owner][type]' and returns a json array like:
   * 
   * [
   *   {"qtype":"A", "qname":"www.example.com", "content":"203.0.113.2", "ttl": 60},
   *   {"qtype":"A", "qname":"www.example.com", "content":"192.0.2.2", "ttl": 60}
   * ]
   * 
   * @param node 
   * @return nlohmann::json::array_t 
   */
  nlohmann::json::array_t getRecordsFromRRSetNode(const libyang::S_Data_Node &node);

  /**
   * @brief Get a new Sysrepo session
   * 
   * @return sysrepo::S_Session 
   */
  sysrepo::S_Session getSession() {
    sysrepo::S_Session ret(new sysrepo::Session(d_connection, SR_DS_RUNNING));
    return ret;
  }

  /**
   * @brief Returns the datatree rooted at `/pdns-server:zones/zones` or an error
   * 
   * @param sysrepo::S_Session  optional sysrepo session to use
   * @return libyang::S_Data_Node 
   * @throw sysrepo::sysrepo_exception when a sysrepo error occurs
   */
  libyang::S_Data_Node getZoneTree(sysrepo::S_Session session = nullptr);

  /**
   * @brief Get the Errors from the sysrepo session
   * 
   * @param session 
   * @return sessionErrors 
   * @throw logic_error when session is a nullptr
   * @throw sysrepo_exception when the errors can't be retrieved
   */
  sessionErrors getErrorsFromSession(const sysrepo::S_Session& session);

  /**
   * @brief Decodes a url-encoded string
   * 
   * @param eString 
   * @return std::string 
   */
  std::string urlDecode(std::string &eString);

  /**
   * @brief DomainIDs assigned to zones
   * 
   * These are only valid during the runtime of this program
   * 
   */
  boost::bimap<std::string, uint32_t> d_domainIds;

  /**
   * @brief Get the ID for a domain
   * 
   * @param domain 
   * @return uint32_t 
   */
  uint32_t getDomainID(const std::string& domain);

  /**
   * @brief Get the domain belong to an id
   * 
   * @param id 
   * @return std::string 
   * @throw std::out_of_range when the id is not found
   */
  std::string getDomainFromId(const uint32_t id);

  /**
   * @brief Contains the serial notified for domains
   * 
   * The map is keyed with the domain_id 
   */
  std::map<uint32_t, uint32_t> d_notifiedMasters;

  void logRequest(const Pistache::Rest::Request &request);
  void logRequestResponse(const Pistache::Rest::Request &request, const Pistache::Http::Response &response, const nlohmann::json& ret);

  sysrepo::S_Connection d_connection;
  std::shared_ptr<Http::Endpoint> d_endpoint;
  Rest::Router d_router;
  const Http::Mime::MediaType s_application_json_mediatype{Http::Mime::MediaType::fromString("application/json")};
};
} // namespace pdns_sysrepo::remote_backend