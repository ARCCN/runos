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

#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>

#include "bits.hh"

namespace runos {
namespace oxm {

template< class T >
struct bit_traits {
    static constexpr size_t nbits = T::nbits;
};

template< >
struct bit_traits<uint8_t> {
    static constexpr size_t nbits = 8;
};

template< > struct bit_traits<uint16_t> {
    static constexpr size_t nbits = 16;
};

template< >
struct bit_traits<uint32_t> {
    static constexpr size_t nbits = 32;
};

template< >
struct bit_traits<uint64_t> {
    static constexpr size_t nbits = 64;
};

} // namespace oxm
} // namespace runos
