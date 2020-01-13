#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::getUpdatedMasters(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
  logRequest(request);
  auto session = getSession();
  auto zones = session->get_subtree("/pdns-server:zones/zones");
  nlohmann::json::array_t allZones;
  for (auto const& zone : zones->tree_for()) {
    auto zoneNode = zone->child(); //  This is /pdns-server:zones/zones[name]/name
    nlohmann::json domainInfo;
    string zoneName;

    {
      auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(zoneNode);
      zoneName = leaf->value_str();
    }

    auto zonetypeXPath = fmt::format("/pdns-server:zones/zones['{}']/zonetype", zoneName);
    auto zonetypeLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(session->get_subtree(zonetypeXPath.c_str()));
    spdlog::trace("zone type={}", zonetypeLeaf->value_str());
    if (std::string(zonetypeLeaf->value_str()) != "master") {
      continue;
    }

    domainInfo["zone"] = zoneName;

    auto domainId = getDomainID(zoneName);
    domainInfo["id"] = domainId;

    auto serialXPath = fmt::format("/pdns-server:zones/zones['{}']/rrset[owner='{}'][type='SOA']/rdata/SOA/serial", zoneName, zoneName);
    auto zoneSOASerialNode = session->get_subtree(serialXPath.c_str());
    auto zoneSOASerial = std::make_shared<libyang::Data_Node_Leaf_List>(zoneSOASerialNode)->value()->uintu32();
    domainInfo["serial"] = zoneSOASerial;

    uint32_t notifiedSerial = 0;
    auto notifiedIt = d_notifiedMasters.find(domainId);
    if (notifiedIt != d_notifiedMasters.end()) {
      notifiedSerial = notifiedIt->second;
    }
    domainInfo["notified_serial"] = notifiedSerial;

    if (notifiedSerial != zoneSOASerial) {
      allZones.push_back(domainInfo);
    }
  }
  nlohmann::json ret = {{"result", allZones}};
  sendResponse(request, response, ret);
}
} // namespace pdns_sysrepo::remote_backend