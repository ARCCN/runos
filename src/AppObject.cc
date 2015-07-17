/*
 * Copyright 2015 Applied Research Center for Computer Networks
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

#include "AppObject.hh"
#include "Common.hh"

#define FORMAT_STRID std::setw(16) << std::setfill('0')

bool operator==(const AppObject& o1, const AppObject& o2)
{ return o1.id() == o2.id(); }

std::string AppObject::id_str() const
{ return uint64_to_string(id()); }

std::string AppObject::uint64_to_string(uint64_t id)
{
    std::ostringstream ss;
    ss << FORMAT_STRID << id;
    return ss.str();
}

time_t AppObject::connectedSince() const
{ return since; }

void AppObject::connectedSince(time_t time)
{ since = time; }
