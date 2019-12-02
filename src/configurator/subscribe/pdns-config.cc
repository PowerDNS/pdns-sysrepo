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

  iter = session->get_changes_iter("/pdns-server:pdns-server/webserver/*");
  change = session->get_change_tree_next(iter);
  while (change != nullptr) {
    string name = change->node()->schema()->name();
    if (name == "address" || name == "port" || name == "api-key") {
      apiConfigChanged = true;
      break;
    }
    change = session->get_change_tree_next(iter);
  }
}
} // namespace pdns_conf