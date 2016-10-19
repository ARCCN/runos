#pragma once

// FIXME: make efficient implementation using custom bitsets

#include <cstddef>
#include <type_traits> // enable_if
#include <bitset>
#include <utility>
#include <stdexcept>
#include <iterator> // reverse iterator
#include <boost/dynamic_bitset.hpp>

namespace runos {
    template<size_t N>
    class bits;
};

namespace runos {
    ///////////////////
    // Static bitset //
    ///////////////////

    template<size_t N = 0>
    class bits : public std::bitset<N> {
    public:
        typedef std::bitset<N> super;

        bits() = default;

        bits(const std::bitset<N> other)
            : super(std::move(other))
        { }

        constexpr explicit bits( unsigned long long val )
            : super(val)
        { }

        template< class CharT, class Traits, class Alloc >
        explicit bits( const std::basic_string<CharT,Traits,Alloc>& str,
                       typename std::basic_string<CharT,Traits,Alloc>::size_type pos = 0,
                       typename std::basic_string<CharT,Traits,Alloc>::size_type n =
                          std::basic_string<CharT,Traits,Alloc>::npos)
            : super(str, pos, n)
        { }

        template< class CharT >
        explicit bits( const CharT* str,
                       typename std::basic_string<CharT>::size_type n =
                            std::basic_string<CharT>::npos,
                       CharT zero = CharT('0'),
                       CharT one = CharT('1'))
            : super(str, n, zero, one)
        { }

        bool operator==(const bits& other) const noexcept
        { return this->super::operator==(other); }

        bool operator!=(const bits& other) const noexcept
        { return this->super::operator!=(other); }

        operator bits<>() const;
    };

    ////////////////////
    // Dynamic bitset //
    ////////////////////
    
    template<>
    class bits<0> : public boost::dynamic_bitset<uint8_t> {
    public:
        typedef boost::dynamic_bitset<uint8_t> super;

        bits(const super other)
            : super(std::move(other))
        { }

        explicit bits(size_t num_bits)
            : super(num_bits)
        { }

        explicit bits(size_t num_bits, unsigned long val)
            : super(num_bits, val)
        { }

        template< class CharT, class Traits, class Alloc >
        explicit bits( const std::basic_string<CharT,Traits,Alloc>& str,
                       typename std::basic_string<CharT,Traits,Alloc>::size_type pos = 0,
                       typename std::basic_string<CharT,Traits,Alloc>::size_type n =
                          std::basic_string<CharT,Traits,Alloc>::npos)
            : super(str, pos, n)
        { }

        template< class CharT >
        explicit bits( const CharT* str,
                       typename std::basic_string<CharT>::size_type n =
                            std::basic_string<CharT>::npos,
                       CharT zero = CharT('0'),
                       CharT one = CharT('1'))
            : super(str, n, zero, one)
        { }

        // serialization (big-endian)
        bits(size_t num_bits, const block_type* buffer)
            : super( std::reverse_iterator<const block_type*>(
                        buffer + (num_bits / bits_per_block)
                               + ((num_bits % bits_per_block) ? 1 : 0)
                     )
                   , std::reverse_iterator<const block_type*>(buffer-1) )
        {
            resize(num_bits);
        }

        bits(size_t num_bits, unsigned long long val);

        // big-endian
        void to_buffer(block_type* buffer) const
        {
            to_block_range(*this, std::reverse_iterator<block_type*>(
                        buffer + num_blocks()));
        }

        template<size_t N, typename = std::enable_if<(N > 0)> >
        explicit operator bits<N>() const
        {
            if ( size() != N )
                throw std::bad_cast();
            std::string ret;
            to_string(*this, ret);
            return bits<N>(ret);
        }
    };

    ////////////////////
    // Implementation //
    ////////////////////
    
    template<size_t N>
    bits<N>::operator bits<>() const
    {
        return bits<>(this->to_string());
    }

    /////////////////////////
    // bits<N> `op` bits<> //
    /////////////////////////

    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bool operator==(const bits<N>& lhs, const bits<>& rhs)
    { return lhs.size() == rhs.size() && lhs == bits<N>(rhs); }

    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bool operator==(const bits<>& lhs, const bits<N>& rhs)
    { return rhs == lhs; }

    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bool operator!=(const bits<N>& lhs, const bits<>& rhs)
    { return lhs.size() != rhs.size() || lhs != bits<N>(rhs); }

    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bool operator!=(const bits<>& lhs, const bits<N>& rhs)
    { return rhs != lhs; }

    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bits<N> operator&(const bits<N>& lhs, const bits<>& rhs)
    { return lhs & bits<N>(rhs); }
    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bits<N> operator&(const bits<>& lhs, const bits<N>& rhs)
    { return rhs & lhs; }

    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bits<N> operator|(const bits<N>& lhs, const bits<>& rhs)
    { return lhs | bits<N>(rhs); }
    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bits<N> operator|(const bits<>& lhs, const bits<N>& rhs)
    { return rhs | lhs; }

    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bits<N> operator^(const bits<N>& lhs, const bits<>& rhs)
    { return lhs ^ bits<N>(rhs); }
    template<size_t N, typename = typename std::enable_if<(N>=1)>::type>
    bits<N> operator^(const bits<>& lhs, const bits<N>& rhs)
    { return rhs ^ lhs; }

    //////////////////////////
    // Return-type adaptors //
    //////////////////////////

    template<size_t N>
    bits<N> operator&(const bits<N>& lhs, const bits<N>& rhs)
    { return bits<N>(static_cast<typename bits<N>::super const &>(lhs) &
                     static_cast<typename bits<N>::super const &>(rhs)); }
    template<size_t N>
    bits<N> operator|(const bits<N>& lhs, const bits<N>& rhs)
    { return bits<N>(static_cast<typename bits<N>::super const &>(lhs) |
                     static_cast<typename bits<N>::super const &>(rhs)); }
    template<size_t N>
    bits<N> operator^(const bits<N>& lhs, const bits<N>& rhs)
    { return bits<N>(static_cast<typename bits<N>::super const &>(lhs) ^
                     static_cast<typename bits<N>::super const &>(rhs)); }

    ////////////////////
    //   Bit casting  //
    ////////////////////
    
    template<class Target, class Source>
    Target bit_cast(const Source);

    template<>
    inline bits<8> bit_cast(uint8_t from)
    { return bits<8>(from); }

    template<>
    inline uint8_t bit_cast(const bits<8> from)
    { return from.to_ulong(); }

    template<>
    inline bits<16> bit_cast(uint16_t from)
    { return bits<16>(from); }

    template<>
    inline uint16_t bit_cast(const bits<16> from)
    { return from.to_ulong(); }

    template<>
    inline bits<32> bit_cast(uint32_t from)
    { return bits<32>(from); }

    template<>
    inline uint32_t bit_cast(const bits<32> from)
    { return from.to_ulong(); }

    template<>
    inline bits<64> bit_cast(uint64_t from)
    { return bits<64>(from); }

    template<>
    inline uint64_t bit_cast(const bits<64> from)
    { return from.to_ullong(); }

    template<class T>
    T&& bit_cast(T&& from)
    { return std::forward<T>(from); }

} // namespace runos

namespace std {
template<size_t N>
struct hash<runos::bits<N>> {
    size_t operator()(const runos::bits<N>& self) const
    {
        return hash<std::bitset<N>>()(self);
    }
};

template<>
struct hash<runos::bits<>> {
    size_t operator()(const runos::bits<>& self) const
    {
        std::string ret;
        boost::to_string(runos::bits<>::super(self), ret);
        return hash<std::string>()(ret);
    }
};
}
