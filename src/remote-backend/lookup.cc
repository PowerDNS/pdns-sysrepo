#include <spdlog/spdlog.h>
// #include <fmt/format.h>
#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend
{
void RemoteBackend::lookup(const Rest::Request& request, Http::ResponseWriter response) {
  auto recordname = request.param(":recordname").as<std::string>();
  recordname = urlDecode(recordname);
  auto recordtype = request.param(":type").as<std::string>();

  string zoneName;
  try {
    zoneName = findBestZone(recordname);
  } catch (const std::out_of_range &e) {
    sendError(response, e.what());
    return;
  }
  if (zoneName.empty()) {
    sendError(response, "No zone for qname");
  }
  nlohmann::json ret = {{"result", nlohmann::json::array()}};

  auto session = getSession();
  auto tree = session->get_subtree("/pdns-server:zones");

  string xpathBase = fmt::format("/pdns-server:zones/zones['{}']/rrset[owner='{}']", zoneName, recordname);
  string xpathRecords = fmt::format("{}[type={}]", xpathBase, recordtype == "ANY" ? "*" : "'" + recordtype + "'");
  spdlog::trace("Attempting to find record node at {}", xpathRecords);
  auto nodeSet = tree->find_path(xpathRecords.c_str());
  auto record = nlohmann::json();

  // Each node here is one rrset, so we need to make N records from the N rdata's
  for (auto const &node : nodeSet->data()) {
    spdlog::trace("Found node in xpath={} node_path={} node_name={}", xpathRecords, node->path(), node->schema()->name());

    // childNode is rrset[owner][type]/owner
    auto childNode = node->child();

    /* Iterate over childNode and its siblings (owner, ttl, type, rdata).
     * As rdata is the last node, we can rely on record being filled properly when handling
     * the rdata.
     */
    for (auto const &rrsetNode : childNode->tree_for()) {
      if ((rrsetNode->schema()->nodetype() & LYS_CONTAINER) && string(rrsetNode->schema()->name()) == "rdata") {
        // rdataNode is rrset[owner][type]/rdata/<type container>
        auto rdataNode = rrsetNode->child();
        if (record["qtype"] == "SOA") {
          // rdataNode is now the 'rrset/rdata/SOA/mname' node
          rdataNode = rdataNode->child();
          string content;
          for (auto const& soaNode : rdataNode->tree_for()) {
            auto soaLeaf = std::make_shared<libyang::Data_Node_Leaf_List>(soaNode);
            content += " ";
            content += soaLeaf->value_str();
          }
          record["content"] = content.substr(1);
          ret["result"].push_back(record);
        } else {
          // rdataNode is now the 'rrset/rdata/<rrset type>/<whatever the node is called>' node
          rdataNode = rdataNode->child();
          if (rdataNode->schema()->nodetype() & LYS_LEAF) {
              auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(rdataNode);
              record["content"] = leaf->value_str();
              ret["result"].push_back(record);
          } else if (rdataNode->schema()->nodetype() & LYS_LEAFLIST) {
            for (auto const &dataNode : rdataNode->tree_for()) {
              auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(dataNode);
              record["content"] = leaf->value_str();
              ret["result"].push_back(record);
            }
          }
        }
        continue;
      }

      if (!(rrsetNode->schema()->nodetype() & (LYS_LEAFLIST | LYS_LEAF))) {
        continue;
      }
      auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(rrsetNode);
      string leafName(rrsetNode->schema()->name());
      if (leafName == "owner") {
        record["qname"] = leaf->value_str();
      }
      if (leafName  == "type") {
        record["qtype"] = leaf->value_str();
      }
      if (leafName  == "ttl") {
        record["ttl"] = leaf->value()->uintu32();
      }
    }
  }
  sendResponse(response, ret);
}
} // namespace pdns_sysrepo::remote_backend