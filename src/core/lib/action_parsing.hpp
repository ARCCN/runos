/*
 * Copyright 2019 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <boost/property_tree/ptree.hpp>
#include <fluid/ofcommon/common.hh>

namespace runos {
namespace rest {
using boost::property_tree::ptree;
  
void parsingAction(fluid_msg::Action* act, std::string prefix, ptree& pt,
                                           ptree& o_coll, ptree& g_coll);
std::string ip_to_string(uint32_t ip);

} //namespace ptree
} //namespace runos
