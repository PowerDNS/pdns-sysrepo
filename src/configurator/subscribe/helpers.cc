#include "../subscribe.hh"
#include "ZonesApi.h"

namespace pdns_conf
{
string ServerConfigCB::getZoneId(const string& zone) {
  if (d_apiClient != nullptr) {
    pdns_api::ZonesApi zonesApiClient(d_apiClient);

    try {
      auto zones = zonesApiClient.listZones("localhost", zone).get();
      if (zones.size() != 1) {
        throw runtime_error(fmt::format("API returned the wrong number of zones ({}) for '{}', expected 1", zones.size(), zone));
      }
      return zones.at(0)->getId();
    }
    catch (const pdns_api::ApiException& e) {
      throw runtime_error(fmt::format("API error looking up Id for {}: {}", zone, e.what()));
    }
  }
  throw runtime_error("in getZoneId, should never get here");
}
} // namespace pdns_conf