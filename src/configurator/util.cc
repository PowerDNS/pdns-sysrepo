#include <libyang/Libyang.hpp>
#include <sysrepo.h>

#include "util.hh"

namespace pdns_conf::util
{
string libyangNodeType2String(const LYS_NODE& node) {
  switch (node) {
  case LYS_UNKNOWN:
    return "uninitialized";
  case LYS_CONTAINER:
    return "container";
  case LYS_CHOICE:
    return "choice";
  case LYS_LEAF:
    return "leaf";
  case LYS_LEAFLIST:
    return "leaf-list";
  case LYS_LIST:
    return "list";
  case LYS_ANYXML:
    return "anyxml";
  case LYS_CASE:
    return "case";
  case LYS_NOTIF:
    return "notification";
  case LYS_RPC:
    return "rpc";
  case LYS_INPUT:
    return "input";
  case LYS_OUTPUT:
    return "output";
  case LYS_GROUPING:
    return "grouping";
  case LYS_USES:
    return "uses";
  case LYS_AUGMENT:
    return "augment";
  case LYS_ACTION:
    return "action";
  case LYS_ANYDATA:
    return "anydata";
  case LYS_EXT:
    return "complex extension";
  }
  return "unknown";
}

string srEvent2String(const sr_event_t &event){
  switch (event)
  {
  case SR_EV_UPDATE:
    return "UPDATE";
  case SR_EV_CHANGE:
    return "CHANGE";
  case SR_EV_DONE:
    return "DONE";
  case SR_EV_ABORT:
    return "ABORT";
  case SR_EV_ENABLED:
    return "ENABLED";
  case SR_EV_RPC:
    return "RPC";
  }
  return "unknown event";
}

string srChangeOper2String(const sr_change_oper_t &changeOper) {
  switch (changeOper) {
  case SR_OP_CREATED:
    return "created";
  case SR_OP_DELETED:
    return "deleted";
  case SR_OP_MODIFIED:
    return "modified";
  case SR_OP_MOVED:
    return "moved";
  }
  return "unknown";
}
} // namespace pdns_conf