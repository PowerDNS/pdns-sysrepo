#pragma once

#include <string>

#include <libyang/Libyang.hpp>
#include <sysrepo.h>

using std::string;


/**
 * @brief This namespace contains a bunch of functions to make life easier
 */
namespace pdns_conf::util
{
/**
 * @brief Get the type of a node as a string
 * 
 * @param node     Node to get the type for
 * @return string 
 */
string libyangNodeType2String(const LYS_NODE& node);

/**
 * @brief Get the name of the sysrepo event
 * 
 * @param event    Event to get the name for
 * @return string 
 */
string srEvent2String(const sr_event_t& event);

string srChangeOper2String(const sr_change_oper_t &changeOper);
} // namespace pdns_conf::util