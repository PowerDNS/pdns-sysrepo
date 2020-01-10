#include "remote-backend.hh"

namespace pdns_sysrepo::remote_backend {
  nlohmann::json::array_t RemoteBackend::getRecordsFromRRSetNode(const libyang::S_Data_Node &node) {
    auto ret = nlohmann::json::array();
    auto nodeSchemaPath = node->schema()->path();
    if (nodeSchemaPath != "/pdns-server:zones/pdns-server:zones/pdns-server:rrset") {
        throw std::range_error(fmt::format("Node {} is not /pdns-server:zones/pdns-server:zones/pdns-server:rrset", nodeSchemaPath));
    }
    // childNode is rrset[owner][type]/owner
    auto childNode = node->child();

    auto record = nlohmann::json();
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
          ret.push_back(record);
        } else {
          // rdataNode is now the 'rrset/rdata/<rrset type>/<whatever the node is called>' node
          rdataNode = rdataNode->child();
          if (rdataNode->schema()->nodetype() & LYS_LEAF) {
              auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(rdataNode);
              record["content"] = leaf->value_str();
              ret.push_back(record);
          } else if (rdataNode->schema()->nodetype() & LYS_LEAFLIST) {
            for (auto const &dataNode : rdataNode->tree_for()) {
              auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(dataNode);
              record["content"] = leaf->value_str();
              ret.push_back(record);
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
    return ret;
  }
}