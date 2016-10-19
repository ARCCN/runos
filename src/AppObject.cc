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

#include <stdio.h>
#include <boost/lexical_cast.hpp>

#include "Common.hh"

bool operator==(const AppObject& o1, const AppObject& o2)
{ return o1.id() == o2.id(); }

std::string AppObject::id_str() const
{ return boost::lexical_cast<std::string>(id()); }

std::string AppObject::uint32_t_ip_to_string(uint32_t ip)
{
    char ret[20];
    unsigned char octet[4]  = {0,0,0,0};
    for (int i = 0; i < 4; i++) {
        octet[i] = ( ip >> (i*8) ) & 0xFF;
    }
    sprintf(ret, "%d.%d.%d.%d", octet[0],octet[1],octet[2],octet[3]);
    return std::string(ret);
}

time_t AppObject::connectedSince() const
{ return since; }

void AppObject::connectedSince(time_t time)
{ since = time; }
