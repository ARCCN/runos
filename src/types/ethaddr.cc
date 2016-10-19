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

#include "ethaddr.hh"

#include <regex>
#include <istream>
#include <boost/format.hpp>

namespace runos {

static unsigned long long parseMAC(const std::string &str)
{
    static const std::string mac_pattern_colon =
        "([[:xdigit:]]{2}):([[:xdigit:]]{2}):"
        "([[:xdigit:]]{2}):([[:xdigit:]]{2}):"
        "([[:xdigit:]]{2}):([[:xdigit:]]{2})";
    
    static const std::string mac_pattern_dash =
        "([[:xdigit:]]{2})-([[:xdigit:]]{2})-"
        "([[:xdigit:]]{2})-([[:xdigit:]]{2})-"
        "([[:xdigit:]]{2})-([[:xdigit:]]{2})";

    static const std::string mac_pattern_dots =
        "^([[:xdigit:]]{2})([[:xdigit:]]{2})."
        "([[:xdigit:]]{2})([[:xdigit:]]{2})."
        "([[:xdigit:]]{2})([[:xdigit:]]{2})";

    static const std::string mac_pattern =
        mac_pattern_colon + '|' + mac_pattern_dash + '|' + mac_pattern_dots;

    static const auto mac_regex =
        std::regex(mac_pattern, std::regex::optimize);

    std::smatch match;
    if (not std::regex_match(str, match, mac_regex)) {
        throw ethaddr::bad_representation("Bad MAC address: " + str);
    }

    size_t base = (match[1].length() > 0) ? 0 : // colon
        ( match[7].length() > 0 ? 6 /* dash */ : 12 /* dots */);

    unsigned long long res = 0;
    for (size_t oct = 1; oct <= 6; ++oct) {
        res <<= 8;
        res |= (uint8_t) std::stoi(match[base+oct], 0, 16);
    }

    return res;
}

ethaddr::ethaddr(const std::string &str)
    : data_(parseMAC(str))
{ }

ethaddr::ethaddr(uint64_t num)
    : data_(num)
{
    if (num & 0xffff000000000000UL) {
        throw bad_representation("non-zero uint64_t padding of 48-bit mac");
    }
}

ethaddr::ethaddr(const bytes_type &octets) noexcept
    : data_( ((unsigned long long) octets[0] << 40ULL) |
             ((unsigned long long) octets[1] << 32ULL) |
             ((unsigned long long) octets[2] << 24ULL) |
             ((unsigned long long) octets[3] << 16ULL) |
             ((unsigned long long) octets[4] << 8ULL ) |
             ((unsigned long long) octets[5]) )
{
}

ethaddr::bytes_type ethaddr::to_octets() const noexcept
{
    uint64_t num = to_number();
    return bytes_type{{
        (uint8_t) ((num & 0xff0000000000ULL) >> 40ULL),
        (uint8_t) ((num & 0xff00000000ULL) >> 32ULL),
        (uint8_t) ((num & 0xff000000ULL) >> 24ULL),
        (uint8_t) ((num & 0xff0000ULL) >> 16ULL),
        (uint8_t) ((num & 0xff00ULL) >> 8ULL),
        (uint8_t) ((num & 0xffULL) >> 0ULL)
    }};
}

bool is_broadcast(const ethaddr& addr) noexcept
{
    return addr.data_.all();
}

bool is_multicast(const ethaddr& addr) noexcept
{
    return addr.data_[48 - 8];
}

bool is_unicast(const ethaddr& addr) noexcept
{
    return not addr.data_[48 - 8];
}

bool ethaddr::is_locally_administered() const noexcept
{
    return data_[48 - 7];
}

std::ostream& operator<<(std::ostream& out, const ethaddr& addr)
{
    const auto& data = addr.to_octets();
    return out << boost::format("%02x:%02x:%02x:%02x:%02x:%02x")
        % (unsigned)data[0] % (unsigned)data[1] % (unsigned)data[2]
        % (unsigned)data[3] % (unsigned)data[4] % (unsigned)data[5];
}

std::istream& operator>>(std::istream& in, ethaddr& addr)
{
    enum {
        mac_str_size = 2*ethaddr::nbytes /* octets */ +
            (ethaddr::nbytes-1) /* colon */ + 1 /* \0 */
    };
    char buf[mac_str_size]; buf[mac_str_size-1] = '\0';

    std::istream::sentry s(in);

    if (s) try {
        in.read(buf, mac_str_size - 1);
        addr = ethaddr(buf);
    } catch (ethaddr::bad_representation) {
        in.setstate(std::ios::failbit);
    }

    return in;
}

}  // namespace runos

using runos::ethaddr;
size_t std::hash<ethaddr>::operator() (const ethaddr& addr) const
{
    return std::hash<unsigned long long>()(addr.to_number());
}
