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
