#include "test-common.hh"

#include "configurator/subscribe.hh"

#include "httpmockserver/mock_server.h"
#include "httpmockserver/test_environment.h"

#define APIKEY "foo"

class HTTPMock: public httpmock::MockServer {
  public:
    /// Create HTTP server on port 9200
    explicit HTTPMock(int port = 9200): MockServer(port) {}
  private:

    /// Handler called by MockServer on HTTP request.
    Response responseHandler(
            const std::string &url,
            const std::string &method,
            const std::string &data,
            const std::vector<UrlArg> &urlArguments,
            const std::vector<Header> &headers)
    {
        for (auto const &h : headers) {
            if (h.key == "X-API-Key" && h.value != APIKEY) {
                return Response(401, "{\"error\": \"not authorized\"");
            }
        }

        if (method == "POST" && matchesPrefix(url, "/api/v1/servers/localhost/zones")) {
            return Response(500, "Fake HTTP response");
        }
        // Return "URI not found" for the undefined methods
        return Response(404, "Not Found");
    }

    /// Return true if \p url starts with \p str.
    bool matchesPrefix(const std::string &url, const std::string &str) const {
        return url.substr(0, str.size()) == str;
    }
};

/// Server started in the main().
static httpmock::TestEnvironment<httpmock::MockServerHolder>* mock_server_env = nullptr;
static std::shared_ptr<pdns_api::ApiClient> apiClient;

TEST(zone_test, add) {
    auto cb = pdns_conf::ServerConfigCB({{}}, apiClient);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  if (argc != 2) {
    cerr<<"Please prove the path to the yang directory"<<endl;
    return 1;
  }
  yangDir = argv[1];

  ::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(httpmock::createMockServerEnvironment<HTTPMock>(9200));
  mock_server_env = dynamic_cast<httpmock::TestEnvironment<httpmock::MockServerHolder>*>(env);

  std::shared_ptr<pdns_api::ApiConfiguration> apiConfig(new pdns_api::ApiConfiguration);
  apiConfig->setBaseUrl("http://127.0.0.1:" + std::to_string(mock_server_env->getMock()->getPort()) + "/api/v1");
  apiConfig->setApiKey("X-API-Key", APIKEY);
  apiConfig->setUserAgent("pdns-sysrepo/" + string(VERSION));
  apiClient = make_shared<pdns_api::ApiClient>(pdns_api::ApiClient(apiConfig));

  return RUN_ALL_TESTS();
}