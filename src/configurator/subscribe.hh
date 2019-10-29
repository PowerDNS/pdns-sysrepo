#include <sysrepo-cpp/Session.hpp>
#include <spdlog/spdlog.h>

using namespace std;

namespace pdns_conf
{
sysrepo::S_Callback getServerConfigCB(const string& fpath);

class ServerConfigCB : public sysrepo::Callback
{
public:
  ServerConfigCB(const map<string, string>& privData) :
    sysrepo::Callback(),
    privData(privData){};
  ~ServerConfigCB(){};
  int module_change(sysrepo::S_Session session, const char* module_name,
    const char* xpath, sr_event_t event,
    uint32_t request_id, void* private_data) override;

private:
  map<string, string> privData;
};
} // namespace pdns_conf