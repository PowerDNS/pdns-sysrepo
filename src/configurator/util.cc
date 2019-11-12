#include <libyang/Libyang.hpp>
#include <sysrepo.h>

#include "util.hh"

namespace pdns_conf
{
  /*
string libyangType2String(const libyang::S_Attr &attr) {
  switch (attr->value_type()) {
  case LY_TYPE_DER:
    return "derived";
  case LY_TYPE_BINARY:
    return "binary";
  case LY_TYPE_BITS:
    return "bits";
  case LY_TYPE_BOOL:
    return "boolean";
  case LY_TYPE_DEC64:
    return "float64";
  case LY_TYPE_EMPTY:
    return "empty";
  case LY_TYPE_ENUM:
    return "enum";
  case LY_TYPE_IDENT:
    return "ident";
  case LY_TYPE_INST:
    return "reference";
  case LY_TYPE_INT16:
    return "int16";
  case LY_TYPE_INT32:
    return "int32";
  case LY_TYPE_INT64:
    return "int64";
  case LY_TYPE_INT8:
    return "int8";
  case LY_TYPE_LEAFREF:
    return "leafref";
  case LY_TYPE_STRING:
    return "string";
  case LY_TYPE_UINT16:
    return "uint16";
  case LY_TYPE_UINT32:
    return "uint32";
  case LY_TYPE_UINT64:
    return "int64";
  case LY_TYPE_UINT8:
    return "uint8";
  case LY_TYPE_UNION:
    return "union";
  case LY_TYPE_UNKNOWN:
    return "unknown";
  }
  return "unknown type";
}
*/

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
} // namespace pdns_conf