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

#include "bits.hh"

namespace runos {class ipv4addr;}

namespace std{
template<>
struct hash<runos::ipv4addr>{
    size_t operator()(const runos::ipv4addr& addr) const;
};
}//namespace std

namespace runos {

class ipv4addr{
public:
    static constexpr size_t             nbits = 32;
    static constexpr size_t             nbytes = 4;
    typedef bits<nbits>                 bits_type;
    typedef std::array<uint8_t, nbytes> bytes_type;

    struct bad_representation : std::domain_error {
        using std::domain_error::domain_error;
    };

    //all octets zero addr
    ipv4addr() = default;

    //fron string
    ipv4addr(const std::string &str);
    ipv4addr(const char* str)
        : ipv4addr(std::string(str))
    { }

    //from octets
    ipv4addr(const bytes_type &octets) noexcept;

    //from bits
    ipv4addr(const bits_type bits) noexcept
        : data_(std::move(bits))
    { }

    // Construct IPv4 Address from number
    ipv4addr(uint32_t num);

    bytes_type to_octets() const noexcept;

    uint32_t to_number() const noexcept
    { return data_.to_ulong(); }

    bits_type to_bits() const noexcept
    { return data_; }

    friend bool operator==(const ipv4addr& lhs, const ipv4addr& rhs)
    { return lhs.data_ == rhs.data_; }

    friend bool operator!=(const ipv4addr& lhs, const ipv4addr&rhs)
    { return lhs.data_ != rhs.data_; }

    friend bool is_private(const ipv4addr&) noexcept;
    friend bool is_loopback(const ipv4addr&) noexcept;
    friend bool is_multicast(const ipv4addr&) noexcept;
    friend bool is_broadcast(const ipv4addr&) noexcept;

private:
    bits_type data_;
    friend std::ostream& operator<<(std::ostream&, const ipv4addr&);
    // TODO friend std::istream& operator>>(std::istream&,ipv4addr&);
};

template<>
inline bits<32> bit_cast(const ipv4addr from)
{ return bits<32>(from.to_bits()); }

template<>
inline ipv4addr bit_cast(const bits<32> from)
{ return ipv4addr(from); }

}//namespace runos
