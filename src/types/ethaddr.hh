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
#include <array>
#include <functional>  // for hash
#include <string>
#include <stdexcept>

#include "bits.hh"

namespace runos { class ethaddr; }

namespace std {
template<>
struct hash<runos::ethaddr> {
    size_t operator()(const runos::ethaddr& addr) const;
};
}  // namespace std

namespace runos {

class ethaddr {
public:
    static constexpr size_t             nbits  = 48;
    static constexpr size_t             nbytes = 6;
    typedef bits<nbits>                 bits_type;
    typedef std::array<uint8_t, nbytes> bytes_type;

    struct bad_representation : std::domain_error {
        using std::domain_error::domain_error;
    };

    // all octets zero addr
    ethaddr() = default;

    // from string
    ethaddr(const std::string &str);
    ethaddr(const char* str)
        : ethaddr(std::string(str))
    { }

    // from octets
    ethaddr(const bytes_type &octets) noexcept;
    
    // from bits
    ethaddr(const bits_type bits) noexcept
        : data_(std::move(bits))
    { }

    // Construct MAC from number, i.e. 0x112233445566 -> 11:22:33:44:55:66
    ethaddr(uint64_t num);

    // Return MAC address data in network byte order
    bytes_type to_octets() const noexcept;

    // Return a 64-bit number in the same format as in ethaddr(uint64_t)
    uint64_t to_number() const noexcept
    { return data_.to_ullong(); }

    // Return bits numbered in little-endian order
    bits_type to_bits() const noexcept
    { return data_; }

    friend bool operator== (const ethaddr& lhs, const ethaddr& rhs) noexcept
    { return lhs.data_ == rhs.data_; }

    friend bool operator!= (const ethaddr& lhs, const ethaddr& rhs) noexcept
    { return lhs.data_ != rhs.data_; }

    friend size_t std::hash<ethaddr>::operator() (const ethaddr& addr) const;

    // True if MAC adderss is set by user
    bool is_locally_administered() const noexcept;
    //
    friend bool is_broadcast(const ethaddr& addr) noexcept;
    friend bool is_multicast(const ethaddr& addr) noexcept;
    friend bool is_unicast(const ethaddr& addr) noexcept;

    friend std::ostream& operator<<
        (std::ostream& out, const ethaddr& addr);
    friend std::istream& operator>>
        (std::istream& in, ethaddr& addr);

private:
    bits_type data_;
};

template<>
inline bits<48> bit_cast(const ethaddr from)
{ return bits<48>(from.to_bits()); }

template<>
inline ethaddr bit_cast(const bits<48> from)
{ return ethaddr(from); }

}  // namespace runos
