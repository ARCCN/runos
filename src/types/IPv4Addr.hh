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

namespace runos {class IPv4Addr;}

namespace std{
template<>
struct hash<runos::IPv4Addr>{
    size_t operator()(const runos::IPv4Addr& addr) const;
};
}//namespace std

namespace runos {

class IPv4Addr{
public:
    static constexpr size_t             nbits = 32;
    static constexpr size_t             nbytes = 4;
    typedef bits<nbits>                 bits_type;
    typedef std::array<uint8_t, nbytes> bytes_type;

    struct bad_representation : std::domain_error {
        using std::domain_error::domain_error;
    };

    //all octets zero addr
    IPv4Addr() = default;

    //fron string
    IPv4Addr(const std::string &str);
    IPv4Addr(const char* str)
        : IPv4Addr(std::string(str))
    { }

    //from octets
    IPv4Addr(const bytes_type &octets) noexcept;

    //from bits
    IPv4Addr(const bits_type bits) noexcept
        : data_(std::move(bits))
    { }

    // Construct IPv4 Address from number
    IPv4Addr(uint32_t num);

    bytes_type to_octets() const noexcept;

    uint32_t to_number() const noexcept
    { return data_.to_ulong(); }

    bits_type to_bits() const noexcept
    { return data_; }

    friend bool operator==(const IPv4Addr& lhs, const IPv4Addr& rhs)
    { return lhs.data_ == rhs.data_; }

    friend bool operator!=(const IPv4Addr& lhs, const IPv4Addr&rhs)
    { return lhs.data_ != rhs.data_; }

    friend bool is_private(const IPv4Addr&) noexcept;
    friend bool is_loopback(const IPv4Addr&) noexcept;
    friend bool is_multicast(const IPv4Addr&) noexcept;
    friend bool is_broadcast(const IPv4Addr&) noexcept;

private:
    bits_type data_;
    friend std::ostream& operator<<(std::ostream&, const IPv4Addr&);
    // TODO friend std::istream& operator>>(std::istream&,IPv4Addr&);
};

template<>
inline bits<32> bit_cast(const IPv4Addr from)
{ return bits<32>(from.to_bits()); }

template<>
inline IPv4Addr bit_cast(const bits<32> from)
{ return IPv4Addr(from); }

}//namespace runos
