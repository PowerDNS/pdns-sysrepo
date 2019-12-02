#include "../subscribe.hh"

namespace pdns_conf
{
void ServerConfigCB::changeConfigUpdate(sysrepo::S_Session& session, const uint32_t request_id) {
  auto iter = session->get_changes_iter("/pdns-server:pdns-server//*");
  auto change = session->get_change_tree_next(iter);

  if (change != nullptr) {
    pdnsConfigChanged = true;
    auto fpath = tmpFile(request_id);

    auto sess = static_pointer_cast<sr::Session>(session);

    // The session already has the new datastore values
    PdnsServerConfig c(sess->getConfigTree());
    c.writeToFile(fpath);
  }
}
} // namespace pdns_conf