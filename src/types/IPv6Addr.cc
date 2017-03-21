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

#include "IPv6Addr.hh"

#include <tuple>
#include <regex>
#include <boost/format.hpp>

namespace runos {

static std::pair<uint64_t, uint64_t> parseIPv6(const std::string &_str)
{
    static const std::string validation =
    "([[:xdigit:]]{1,4}:){7}[[:xdigit:]]{1,4}|"
    // 1:2:3:4:5:6:7:8
    "([[:xdigit:]]{1,4}:){1,7}:|"
    // 1::  1:2:3:4:5:6:7::
    "([[:xdigit:]]{1,4}:){1,6}:[[:xdigit:]]{1,4}|"
    // 1::8  1:2:3:4:5:6::8  1:2:3:4:5:6::8
    "([[:xdigit:]]{1,4}:){1,5}(:[[:xdigit:]]{1,4}){1,2}|"
    // 1::7:8  1:2:3:4:5::7:8  1:2:3:4:5::8
    "([[:xdigit:]]{1,4}:){1,4}(:[[:xdigit:]]{1,4}){1,3}|"
    // 1::6:7:8 1:2:3:4::6:7:8  1:2:3:4::8
    "([[:xdigit:]]{1,4}:){1,3}(:[[:xdigit:]]{1,4}){1,4}|"
    // 1::5:6:7:8  1:2:3::5:6:7:8  1:2:3::8
    "([[:xdigit:]]{1,4}:){1,2}(:[[:xdigit:]]{1,4}){1,5}|"
    // 1::4:5:6:7:8  1:2::4:5:6:7:8  1:2::8
    "[[:xdigit:]]{1,4}:((:[[:xdigit:]]{1,4}){1,6})|"
    // 1::3:4:5:6:7:8  1::3:4:5:6:7:8  1::8
    ":((:[[:xdigit:]]{1,4}){1,7}|:)";
    // ::2:3:4:5:6:7:8  ::2:3:4:5:6:7:8 ::8  ::
    static const auto regex_validation = std::regex(validation, std::regex::optimize);
    if (not std::regex_match(_str, regex_validation)){
        throw IPv6Addr::bad_representation("Bad IPv6 address: " + _str);
    }
    std::string str = _str;
    auto n = std::count(str.begin(), str.end(),':');
    if (n < 7){
        auto pos = str.find("::");
        str.insert(pos, 7 - n, ':');
    }
    static const std::string patter =
         "([[:xdigit:]]{0,4}):([[:xdigit:]]{0,4}):"
         "([[:xdigit:]]{0,4}):([[:xdigit:]]{0,4}):"
         "([[:xdigit:]]{0,4}):([[:xdigit:]]{0,4}):"
         "([[:xdigit:]]{0,4}):([[:xdigit:]]{0,4})";
    static const auto regex = std::regex(patter, std::regex::optimize);

    std::smatch match;
    if (not std::regex_match(str, match, regex)){
        throw IPv6Addr::bad_representation("Bad IPv6 address: " + str);
    }

    uint64_t one = 0, two = 0;
    for (size_t oct = 1; oct <= 4; ++oct){
        one <<= 16;
        if (match[oct].length() > 0 ){
            one |= (uint16_t) std::stoi(match[oct], 0, 16);
        }
    }
    for (size_t oct = 5; oct <= 8; ++oct){
        two <<= 16;
        if (match[oct].length() > 0 ){
            two |= (uint16_t) std::stoi(match[oct], 0, 16);
        }
    }
    return std::make_pair(one, two);
}

IPv6Addr::IPv6Addr(const std::string &str)
     : IPv6Addr(parseIPv6(str))
{ }
IPv6Addr::IPv6Addr(std::pair<uint64_t, uint64_t> num)
    : data_(num.first)
{
    data_ <<= 64;
    data_ |= num.second;
}

std::pair <uint64_t, uint64_t> IPv6Addr::to_number() const noexcept
{
    uint64_t one = 0, two = 0;
    one = data_.to_ullong();
    two = (data_ >> 64).to_ullong();
    return std::make_pair(one, two);
}

IPv6Addr::hextets_type IPv6Addr::to_hextets() const noexcept
{
    uint64_t one, two;
    std::tie(one, two) = to_number();
    return hextets_type{{
        (uint16_t) ((one & 0xffff000000000000ULL) >> 48ULL),
        (uint16_t) ((one & 0xffff00000000ULL)     >> 32ULL),
        (uint16_t) ((one & 0xffff0000ULL)         >> 16ULL),
        (uint16_t) ((one & 0xffffULL)             >>  0ULL),

        (uint16_t) ((two & 0xffff000000000000ULL) >> 48ULL),
        (uint16_t) ((two & 0xffff00000000ULL)     >> 32ULL),
        (uint16_t) ((two & 0xffff0000ULL)         >> 16ULL),
        (uint16_t) ((two & 0xffffULL)             >>  0ULL),
    }};

}

IPv6Addr::bytes_type IPv6Addr::to_octets() const noexcept
{
    uint64_t one, two;
    std::tie(one, two) = to_number();
    return bytes_type{{
        (uint8_t) ((one & 0xff00000000000000ULL) >> 54ULL),
        (uint8_t) ((one &   0xff000000000000ULL) >> 48ULL),
        (uint8_t) ((one &     0xff0000000000ULL) >> 40ULL),
        (uint8_t) ((one &       0xff00000000ULL) >> 32ULL),
        (uint8_t) ((one &         0xff000000ULL) >> 24ULL),
        (uint8_t) ((one &           0xff0000ULL) >> 16ULL),
        (uint8_t) ((one &             0xff00ULL) >>  8ULL),
        (uint8_t) ((one &               0xffULL) >>  0ULL),

        (uint8_t) ((two & 0xff00000000000000ULL) >> 54ULL),
        (uint8_t) ((two &   0xff000000000000ULL) >> 48ULL),
        (uint8_t) ((two &     0xff0000000000ULL) >> 40ULL),
        (uint8_t) ((two &       0xff00000000ULL) >> 32ULL),
        (uint8_t) ((two &         0xff000000ULL) >> 24ULL),
        (uint8_t) ((two &           0xff0000ULL) >> 16ULL),
        (uint8_t) ((two &             0xff00ULL) >>  8ULL),
        (uint8_t) ((two &               0xffULL) >>  0ULL),
    }};
}

std::ostream& operator<<(std::ostream& out, const IPv6Addr& addr)
{
    //TODO: pretty print
    const auto& data = addr.to_hextets();
    return out << boost::format("%x:%x:%x:%x:%x:%x:%x:%x")
    % (unsigned long)data[0] % (unsigned long)data[1]
    % (unsigned long)data[2] % (unsigned long)data[3]
    % (unsigned long)data[4] % (unsigned long)data[5]
    % (unsigned long)data[6] % (unsigned long)data[7];
}

} //namespace runos

using runos::IPv6Addr;
size_t std::hash<IPv6Addr>::operator() (const IPv6Addr& addr) const
{
    uint64_t one, two;
    std::tie(one, two) = addr.to_number();
    return std::hash<unsigned long long>()(one ^ two);
}
