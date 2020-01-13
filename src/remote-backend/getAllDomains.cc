#include <spdlog/spdlog.h>

#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::getAllDomains(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
  logRequest(request);
  nlohmann::json ret = {{"result", nlohmann::json::array()}};
  auto session = getSession();

  libyang::S_Data_Node tree;
  try {
    tree = session->get_subtree("/pdns-server:zones/zones");
  } catch(const sysrepo::sysrepo_exception &e) {
    spdlog::warn("RemoteBackend::getAllDomains: Unable to retrieve zones");
    auto errors = session->get_error();
    for (size_t i = 0; i < errors->error_cnt(); i++) {
      string xpath = errors->xpath(i) == nullptr ? "" : errors->xpath(i);
      string message = errors->message(i) == nullptr ? "" : errors->message(i);
      spdlog::warn("Session error xpath={}: {}", xpath, message);
    }
    sendError(request, response, "Unable to retrieve data");
    return;
  }

  string zoneName, kind;
  for (auto const &node : tree->tree_dfs()) {
    if (node->schema()->nodetype() != LYS_LEAF) {
      continue;
    }
    auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
    string leafName(leaf->schema()->name());
    spdlog::trace("path={} name={} value={}", leaf->path(), leaf->schema()->name(), leaf->value_str());

    if (leafName == "name") {
      if (!zoneName.empty()) {
        ret["result"].push_back(makeDomainInfo(zoneName, kind));
      }
      zoneName = leaf->value_str();
    }
    if (leafName == "zonetype") {
      kind = leaf->value_str();
    }
  }
  if (!zoneName.empty()) {
    ret["result"].push_back(makeDomainInfo(zoneName, kind));
  }
  sendResponse(request, response, ret);
}
} // namespace pdns_sysrepo::remote_backend