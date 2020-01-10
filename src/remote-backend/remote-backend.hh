#include <spdlog/spdlog.h>
#include <sysrepo-cpp/Session.hpp>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/net.h>
#include <nlohmann/json.hpp>

#pragma once

namespace pdns_sysrepo::remote_backend
{

namespace Http = Pistache::Http;
namespace Rest = Pistache::Rest;
using std::string;

class RemoteBackend
{
public:
  RemoteBackend(sysrepo::S_Connection& connection, const Pistache::Address& addr) :
    d_connection(connection),
    d_endpoint(std::make_shared<Http::Endpoint>(addr)) {
    Rest::Routes::Get(d_router, "/dns/lookup/:recordname/:type", Rest::Routes::bind(&RemoteBackend::lookup, this));
    Rest::Routes::Get(d_router, "/dns/getAllDomains", Rest::Routes::bind(&RemoteBackend::getAllDomains, this));
    Rest::Routes::NotFound(d_router, Rest::Routes::bind(&RemoteBackend::notFound, this));

    auto opts = Http::Endpoint::options().threads(4).flags(Pistache::Tcp::Options::ReuseAddr);
    d_endpoint->init(opts);
  };

  ~RemoteBackend() {
    try {
      d_endpoint->shutdown();
    } catch (const std::exception &e) {
      spdlog::warn("Exception in RemoteBackend destructor: {}", e.what());
    }
  };

  void start() {
    d_endpoint->setHandler(d_router.handler());
    // This ensures we actually return
    d_endpoint->serveThreaded();
  };

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
  void sendError(Http::ResponseWriter& response, const std::string& error, const Http::Code &code = Http::Code::Internal_Server_Error);

  /**
   * @brief Send an HTTP 200 response with ret
   * 
   * @param response 
   * @param ret
   */
  void sendResponse(Http::ResponseWriter& response, const nlohmann::json& ret);

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
   * @brief Get a new Sysrepo session
   * 
   * @return sysrepo::S_Session 
   */
  sysrepo::S_Session getSession() {
    sysrepo::S_Session ret(new sysrepo::Session(d_connection, SR_DS_RUNNING));
    return ret;
  }

  /**
   * @brief Decodes a url-encoded string
   * 
   * @param eString 
   * @return std::string 
   */
  std::string urlDecode(std::string &eString);

  sysrepo::S_Connection d_connection;
  std::shared_ptr<Http::Endpoint> d_endpoint;
  Rest::Router d_router;
  const Http::Mime::MediaType s_application_json_mediatype{Http::Mime::MediaType::fromString("application/json")};
};
} // namespace pdns_sysrepo::remote_backend