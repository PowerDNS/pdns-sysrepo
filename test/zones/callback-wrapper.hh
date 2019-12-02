#pragma once

#include "configurator/subscribe.hh"

class TestServerConfigCB : public pdns_conf::ServerConfigCB
{
public:
  /**
   * @brief Overwritten ServerConfigCB, allows us to call protected methods directly
   * 
   * @param privData 
   * @param apiClient 
   */
  TestServerConfigCB(const map<string, string>& privData, shared_ptr<pdns_api::ApiClient>& apiClient) :
    pdns_conf::ServerConfigCB(privData, apiClient){};
  ~TestServerConfigCB(){};

  void modifyZoneType(const string& zonename, const string& newType) {
    auto zone = zonesModified[zonename];
    zone.setKind(newType);
    zonesModified[zonename] = zone;
  }

  void addZone(const string &zonename, const string zonetype) {
    auto z = pdns_api_model::Zone();
    z.setKind(zonetype);
    z.setName(zonename);
    zonesCreated.push_back(z);
  }

  void doDoneZoneModify() {
    doneZoneModify();
    zonesModified.clear();
  }

  void doDoneZoneAddAndDelete() {
    doneZoneAddAndDelete();
    zonesCreated.clear();
    zonesRemoved.clear();
  }
};