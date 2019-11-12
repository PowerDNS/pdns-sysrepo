#pragma once

#include <string>

#include <libyang/Libyang.hpp>
#include <sysrepo.h>

using std::string;

namespace pdns_conf
{
string libyangNodeType2String(const LYS_NODE &node);
string srEvent2String(const sr_event_t &event);
} // namespace pdns_conf