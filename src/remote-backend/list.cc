#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend {
void RemoteBackend::list(const Pistache::Rest::Request& request, Http::ResponseWriter response) {
  auto zoneName = request.param(":zone").as<std::string>();
  zoneName = urlDecode(zoneName);

  nlohmann::json::array_t allRecords;

  auto session = getSession();
  auto zoneXPath = fmt::format("/pdns-server:zones/zones[name='{}']", zoneName);

  spdlog::trace("remote_backend/list - Grabbing zone xpath={}", zoneXPath);
  auto tree = session->get_subtree(zoneXPath.c_str());
  spdlog::trace("remote_backend/list - tree path={}", tree->path());

  for (auto const& rrsetNode : tree->child()->tree_for()) {
    if (string(rrsetNode->schema()->name()) != "rrset") {
      continue;
    }
    auto records = getRecordsFromRRSetNode(rrsetNode);
    allRecords.insert(
      allRecords.end(),
      std::make_move_iterator(records.begin()),
      std::make_move_iterator(records.end())
    );
  }
  nlohmann::json ret = {{"result", allRecords}};

  sendResponse(response, ret);
}
}