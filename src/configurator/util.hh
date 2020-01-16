/*
 * Copyright Pieter Lexis <pieter.lexis@powerdns.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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