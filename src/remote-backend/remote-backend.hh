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
  RemoteBackend(sysrepo::S_Session& session, const Pistache::Address& addr) :
    d_session(session),
    d_endpoint(std::make_shared<Http::Endpoint>(addr)) {
    Rest::Routes::Get(d_router, "/dns/lookup/:recordname/:type", Rest::Routes::bind(&RemoteBackend::lookup, this));
    Rest::Routes::NotFound(d_router, Rest::Routes::bind(&RemoteBackend::notFound, this));

    auto opts = Http::Endpoint::options().threads(4);
    d_endpoint->init(opts);
  };

  ~RemoteBackend() {
    d_endpoint->shutdown();
  };

  void start() {
    d_endpoint->setHandler(d_router.handler());
    // This ensures we actually return
    d_endpoint->serveThreaded();
  };

  void stop() {
    d_endpoint->shutdown();
  };

protected:
  /**
   * @brief Implements the lookup endpoint
   * 
   * @param request 
   * @param response 
   */
  void lookup(const Pistache::Rest::Request& request, Http::ResponseWriter response);

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

  sysrepo::S_Session d_session;
  std::shared_ptr<Http::Endpoint> d_endpoint;
  Rest::Router d_router;
  const Http::Mime::MediaType s_application_json_mediatype{Http::Mime::MediaType::fromString("application/json")};
};
} // namespace pdns_sysrepo::remote_backend