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

#include "ipv4addr.hh"

#include <regex>
#include <boost/format.hpp>

#include <mutex>

namespace runos {

static unsigned long parseipv4address(const std::string &str)
{
    static const std::string ipv4_pattern =
        "([[:digit:]]{1,3}).([[:digit:]]{1,3})."
        "([[:digit:]]{1,3}).([[:digit:]]{1,3})";

    static const auto ipv4_regex =
        std::regex(ipv4_pattern, std::regex::optimize);

    std::smatch match;

    if (not std::regex_match(str, match, ipv4_regex)) {
        throw ipv4addr::bad_representation("Bad IPv4 address: " + str);
    }

    unsigned long res = 0;
    for (size_t oct = 1; oct <= 4; ++oct) {
        res <<= 8;
        res |= (uint8_t) std::stoi(match[oct], 0, 10);
    }

    return res;
}

static bool compare_prefix(const std::string &pattern,
                           const ipv4addr::bits_type &value,
                           uint8_t subnet)
{
    ipv4addr::bits_type bit_pattern(parseipv4address(pattern));
    uint8_t shift = 32 - subnet;
    return bit_pattern >> shift == value >> shift;
}


ipv4addr::ipv4addr(const std::string &str)
    : data_(parseipv4address(str))
{ }

ipv4addr::ipv4addr(uint32_t num)
    : data_(num)
{ }

ipv4addr::ipv4addr(const bytes_type &octets) noexcept
    :data_( ((unsigned long) octets[0] << 24UL) |
            ((unsigned long) octets[1] << 16UL) |
            ((unsigned long) octets[2] << 8UL ) |
            ((unsigned long) octets[3]) )
{ }

ipv4addr::bytes_type ipv4addr::to_octets() const noexcept
{
    uint32_t num = to_number();
    return bytes_type{{
        (uint8_t) ((num & 0xff000000UL) >> 24UL),
        (uint8_t) ((num & 0xff0000UL)   >> 16UL),
        (uint8_t) ((num & 0xff00UL)     >> 8UL ),
        (uint8_t) ((num & 0xffUL)              ),
    }};
}

std::ostream& operator<<(std::ostream &out, const ipv4addr &ipv4)
{
    static boost::format static_format("%u.%u.%u.%u");
    boost::format format(static_format); // boost::format is not thread safety
    const auto& data = ipv4.to_octets();
    return out << format % unsigned(data[0]) % unsigned(data[1])
                         % unsigned(data[2]) % unsigned(data[3]);
}

bool is_private(const ipv4addr &addr) noexcept
{
    return compare_prefix("10.0.0.0", addr.data_, 8)
        || compare_prefix("172.16.0.0", addr.data_, 12)
        || compare_prefix("192.168.0.0.", addr.data_, 16);
}

bool is_loopback(const ipv4addr &addr) noexcept
{
    return compare_prefix("127.0.0.0", addr.data_, 8);
}

bool is_multicast(const ipv4addr &addr) noexcept
{
    return compare_prefix("224.0.0.0", addr.data_, 4);
}

bool is_broadcast(const ipv4addr &addr) noexcept
{
    return addr.data_.all();
}

}// namespace runos

using runos::ipv4addr;
size_t std::hash<ipv4addr>::operator()(const ipv4addr& addr) const
{
    return std::hash<unsigned long>()(addr.to_number());
}

