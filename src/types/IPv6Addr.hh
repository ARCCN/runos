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

#pragma once

#include <cstdint>
#include <functional>  // for hash
#include <string>
#include <stdexcept>
#include <tuple>

#include "bits.hh"

namespace runos {class IPv6Addr; }


namespace std {
template<>
struct hash<runos::IPv6Addr> {
    size_t operator()(const runos::IPv6Addr& addr) const;
};
}  // namespace std

namespace runos {

class IPv6Addr {
public:
    static constexpr size_t                nbits  = 128;
    static constexpr size_t                nbytes = 16;
    typedef bits<nbits>                    bits_type;
    typedef std::array<uint8_t, nbytes>    bytes_type;
    typedef std::array<uint16_t, nbytes/2> hextets_type;
    struct bad_representation : std::domain_error {
        using std::domain_error::domain_error;
    };

    // all octets zero addr
    IPv6Addr() = default;

    // from bits
    IPv6Addr(const bits_type bits) noexcept
        : data_(std::move(bits))
    { }

    // from string, be careful.
        //wrong parsing ipv6 string. "::" means ":0000:" and nothing else
    IPv6Addr(const std::string &str);
    IPv6Addr(const char* str)
        : IPv6Addr(std::string(str))
    { }

    // from octets
    //IPv6Addr(const bytes_type &octets) noexcept; TODO

    // Construct IPv6 Address from number
    IPv6Addr(std::pair<uint64_t, uint64_t> num);

    // Return IPv6 address data in network two byte order
    hextets_type to_hextets() const noexcept;

    // Return IPv6 address data in byte order
    bytes_type to_octets() const noexcept;

    // return a pair, that represents 128 bits ipv6 address
    std::pair<uint64_t, uint64_t> to_number() const noexcept;

    bits_type to_bits() const noexcept
    { return data_; }

    friend bool operator== (const IPv6Addr& lhs, const IPv6Addr& rhs) noexcept
    { return lhs.data_ == rhs.data_; }

    friend bool operator!= (const IPv6Addr& lhs, const IPv6Addr& rhs) noexcept
    { return lhs.data_ != rhs.data_; }

    friend size_t std::hash<IPv6Addr>::operator() (const IPv6Addr& addr) const;

    friend std::ostream& operator <<
        (std::ostream& out, const IPv6Addr &addr);

private:
    bits_type data_;

};

template<>
inline bits<128> bit_cast(const IPv6Addr from)
{ return bits<128>(from.to_bits()); }

template<>
inline IPv6Addr bit_cast(const bits<128> from)
{ return IPv6Addr(from); }

} //namespace runos
