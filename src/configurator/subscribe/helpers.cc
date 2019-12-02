#include "../subscribe.hh"
#include "ZonesApi.h"

namespace pdns_conf
{
string ServerConfigCB::getZoneId(const string& zone) {
  pdns_api::ZonesApi zonesApiClient(d_apiClient);

  auto zones = zonesApiClient.listZones("localhost", zone).get();
  if (zones.size() != 1) {
    throw runtime_error(fmt::format("API returned the wrong number of zones ({}) for '{}', expected 1", zones.size(), zone));
  }
  return zones.at(0)->getId();
}

void ServerConfigCB::configureApi(const libyang::S_Data_Node &node) {
  auto apiConfig = make_shared<pdns_api::ApiConfiguration>();

  try {
    auto apiKeyNode = node->find_path("/pdns-server:pdns-server/webserver/api-key")->data().at(0);
    apiConfig->setApiKey("X-API-Key", make_shared<libyang::Data_Node_Leaf_List>(apiKeyNode)->value_str());
    auto addressNode = node->find_path("/pdns-server:pdns-server/webserver/address")->data().at(0);
    auto portNode = node->find_path("/pdns-server:pdns-server/webserver/port")->data().at(0);
    apiConfig->setBaseUrl("http://" + string(make_shared<libyang::Data_Node_Leaf_List>(addressNode)->value_str()) + ":" + string(make_shared<libyang::Data_Node_Leaf_List>(portNode)->value_str()) + "/api/v1");
  }
  catch (const out_of_range &e) {
    // ignore, We implicitly disable the API with the "wrong" config
  }

  apiConfig->setUserAgent("pdns-sysrepo/" + string(VERSION));
  d_apiClient->setConfiguration(apiConfig);
}
} // namespace pdns_conf