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

#include "ipv4addr.hpp"
#include <ostream>
#include <arpa/inet.h>

namespace runos {

std::pair<ipv4addr, bool>
convert(const std::string& rep)
{
    in_addr sys_addr;
    if (1 == inet_pton(AF_INET, rep.c_str(), &sys_addr)) {
        return std::make_pair(ipv4addr(sys_addr.s_addr), true);
    }
    else {
        return std::make_pair(ipv4addr(), false);
    }
}

std::ostream&
operator<<(std::ostream& stream, const ipv4addr& ip)
{
    char buf[sizeof("xxx.xxx.xxx.xxx")];
    in_addr sys_addr = { .s_addr = ip.addr };
    inet_ntop(AF_INET, &sys_addr, buf, sizeof(buf));
    stream << std::string(buf);
    return stream;
}

} // namespace runos
