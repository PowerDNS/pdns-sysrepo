#include <sysrepo-cpp/Session.hpp>
#include <spdlog/spdlog.h>

using namespace std;

namespace pdns_conf {
    sysrepo::S_Callback getServerConfigCB();

    class ServerConfigCB: public sysrepo::Callback {
        public:
        int module_change(sysrepo::S_Session session, const char *module_name,
                          const char *xpath, sr_event_t event,
                          uint32_t request_id, void *private_data) override {
            spdlog::debug("Had callback module_name={} xpath={} event={}",
                (module_name == nullptr)? "": module_name,
                (xpath == nullptr)? "": xpath,
                event);
            return SR_ERR_OK;
        }
    };
}