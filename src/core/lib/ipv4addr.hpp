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

#ifndef _IPV4_ADDR_
#define _IPV4_ADDR_

#include <iosfwd>
#include <string>
#include <utility>
#include <algorithm>
#include <stdint.h>

namespace runos {

//
// Simple representation of IPv4 address
//
struct ipv4addr {
    uint32_t addr;

    ipv4addr(): addr() { }
    ipv4addr(const ipv4addr&) = default;
    explicit ipv4addr(uint32_t raw): addr(raw) { }

    explicit operator uint32_t(void) const { return addr; }

    ipv4addr& operator=(const ipv4addr&) = default;
};

bool operator==(const ipv4addr&, const ipv4addr&);
bool operator!=(const ipv4addr&, const ipv4addr&);

std::pair<ipv4addr, bool> convert(const std::string&);

std::ostream& operator<<(std::ostream&, const ipv4addr&);

//
// Short implementations
//

inline bool operator==(const ipv4addr& left, const ipv4addr& right)
{
    return left.addr == right.addr;
}

inline bool operator!=(const ipv4addr& left, const ipv4addr& right)
{
    return !(left == right);
}

} // namespace runos

namespace std {

template<>
struct hash<runos::ipv4addr>
{
    std::size_t operator()(const runos::ipv4addr& ip) const
    {
        return std::hash<uint32_t>()(ip.addr);
    }
};

} // namespace std

#endif//_IPV4_ADDR_
