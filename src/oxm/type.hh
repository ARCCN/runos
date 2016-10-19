#pragma once

#include <cstdint>
#include <cstddef>
#include <functional> // hash
#include <ostream>

#include <typeinfo>
#include <boost/core/demangle.hpp>

#include "type_fwd.hh"
#include "types/bits.hh"

namespace runos {
namespace oxm {

class type {
    uint16_t _ns;
    uint8_t  _id:7;
    bool     _maskable:1;
    uint16_t _nbits;

    const std::type_info* _cpptype;

public:
    using value_type = bits<>;
    using mask_type = bits<>;

    constexpr type(uint16_t ns,
                   uint8_t id,
                   bool maskable,
                   uint16_t nbits,
                   const std::type_info* cpptype = nullptr) noexcept
        : _ns{ns}
        , _id{id}
        , _maskable{maskable}
        , _nbits{nbits}
        , _cpptype{cpptype}
    { }

    constexpr uint16_t ns() const noexcept { return _ns; }
    constexpr uint8_t id() const noexcept { return _id; }
    constexpr size_t nbits() const noexcept { return _nbits; }
    constexpr bool maskable() const noexcept { return _maskable; }

    constexpr size_t nbytes() const noexcept
    {
        return (nbits() / 8) + ( nbits() % 8 ? 1 : 0 );
    }

    friend constexpr bool operator==(const type& lhs, const type& rhs) noexcept
    { return (lhs.ns() == rhs.ns()) && (lhs.id() == rhs.id()); }

    friend constexpr bool operator!=(const type& lhs, const type& rhs) noexcept
    { return ! (lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& out, const type t)
    {
        if (not t._cpptype) {
            return out << "oxm::type{ns=" << t.ns() << ", id=" << unsigned(t.id())
                       << ", mask=" << t.maskable() 
                       << ", nbits=" << t.nbits() << "}";
        } else {
            return out << boost::core::demangle(t._cpptype->name()) << "{}";
        }
    }
};

template< class Final,
          uint16_t NS,
          uint8_t ID,
          size_t NBITS,
          class ValueType,
          class MaskType = ValueType,
          bool HASMASK = false >
struct define_type : type {
    using value_type = ValueType;
    using mask_type = MaskType;

    constexpr define_type()
        : type{ NS, ID, HASMASK, NBITS, &typeid(Final) }
    { }
};

} // namespace oxm
} // namespace runos

namespace std {
    template<>
    struct hash<runos::oxm::type> {
        size_t operator() (runos::oxm::type t) const noexcept
        {
            return std::hash<uint64_t>()
                (static_cast<uint64_t>(t.ns()) << 8ULL | t.id());
        }
    };
}
