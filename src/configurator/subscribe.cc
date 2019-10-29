#include "subscribe.hh"

using namespace std;

namespace pdns_conf {
    sysrepo::S_Callback getServerConfigCB() {
        sysrepo::S_Callback cb(new ServerConfigCB());
        return cb;
    }
}