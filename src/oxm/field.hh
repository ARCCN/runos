#pragma once

#include <ostream>

#include <boost/exception/errinfo_type_info_name.hpp>
#include <boost/exception/info.hpp>

#include "field_fwd.hh"
#include "types/bits.hh"
#include "types/exception.hh"
#include "type.hh"
#include "errors.hh"

namespace runos {

namespace detail {
    template<class T>
    struct oxm_type_holder {
        constexpr T type() const noexcept
        { return T(); }
    };

    template<>
    struct oxm_type_holder<oxm::type> {
    protected:
        oxm::type m_type;
    public:
        constexpr explicit oxm_type_holder(oxm::type t)
            : m_type(t)
        { }

        constexpr oxm::type type() const noexcept
        { return m_type; }
    };
}

namespace oxm {

namespace detail {

    template<class T>
    using type_holder = ::runos::detail::oxm_type_holder<T>;
    
    template<class T>
    struct bits_type_t {
        using type = bits<T().nbits()>;
    };
    template<>
    struct bits_type_t<type> {
        using type = bits<>;
    };
    template<class T> using bits_type
        = typename bits_type_t<T>::type;

    template<class T>
    struct value_holder {
    protected:
        using value_type
            = typename T::value_type;
        bits_type<T> m_value;

        template<class U>
        friend struct ::runos::oxm::value;
    public:
        explicit value_holder(const T t, bits_type<T> val)
            : m_value(std::move(val))
        {
            if (m_value.size() != t.nbits()) {
                RUNOS_THROW(bad_bit_length()
                    << errinfo_actual_bit_length(m_value.size())
                    << errinfo_good_bit_length(t.nbits())
                    << errinfo_type(t));
            }
        }

        constexpr bits_type<T> value_bits() const
        { return m_value; }
    };

    template<class T>
    struct mask_holder {
    protected:
        using mask_type
            = typename T::mask_type;
        bits_type<T> m_mask;

        template<class U>
        friend struct ::runos::oxm::mask;
    public:
        explicit mask_holder(const T t, bits_type<T> mask)
            : m_mask(std::move(mask))
        {
            if (m_mask.size() != t.nbits()) {
                RUNOS_THROW(bad_bit_length()
                    << errinfo_actual_bit_length(m_mask.size())
                    << errinfo_good_bit_length(t.nbits())
                    << boost::errinfo_type_info_name( typeid(T).name() )
                    << errinfo_type(t));
            }
            if (not t.maskable() && not (m_mask.all() || m_mask.none())) {
                RUNOS_THROW(bad_mask()
                    << boost::errinfo_type_info_name( typeid(T).name() )
                    << errinfo_type(t));
            }
        }

        constexpr bits_type<T> mask_bits() const
        { return m_mask; }

        constexpr bool exact() const noexcept
        { return m_mask.all(); }

        constexpr bool wildcard() const noexcept
        { return m_mask.none(); }

        constexpr bool fuzzy() const noexcept
        { return not(exact() || wildcard()); }
    };

    template<class T1, class T2>
    struct binop_type_func {
        // produces compile-time error
    };

    template<class T>
    struct binop_type_func<T, T> {
        typedef T result_type;
        constexpr result_type operator()(const T, const T) const noexcept
        { return T(); }
    };

    template<class T>
    struct binop_type_func<T, type> {
        typedef T result_type;
        result_type operator()(const T lhs, const type rhs) const
        {
            if (lhs == rhs) return T();
            RUNOS_THROW(bad_operands()
                << boost::errinfo_type_info_name( typeid(T).name() )
                << errinfo_lhs_type(lhs)
                << errinfo_rhs_type(rhs));
        }
    };

    template<class T>
    struct binop_type_func<type, T> {
        typedef T result_type;
        T operator()(const type lhs, const T rhs) const
        {
            if (lhs == rhs) return T();
            RUNOS_THROW(bad_operands()
                << errinfo_lhs_type(lhs)
                << boost::errinfo_type_info_name( typeid(T).name() )
                << errinfo_rhs_type(rhs));
        }
    };

    template<>
    struct binop_type_func<type, type> {
        typedef type result_type;
        type operator()(const type lhs, const type rhs) const
        {
            if (lhs == rhs) return lhs;
            RUNOS_THROW(bad_operands()
                << errinfo_lhs_type(lhs)
                << errinfo_rhs_type(rhs));
        }
    };

    template<class T1, class T2,
             class type1 = decltype(std::declval<T1>().type()),
             class type2 = decltype(std::declval<T2>().type())>
    typename binop_type_func<type1, type2>::result_type
    binop_type(const T1 lhs, const T2 rhs)
    noexcept(noexcept(binop_type_func<type1, type2>()(std::declval<const type1>(),
                                                      std::declval<const type2>())))
    {
        return binop_type_func<type1, type2>()(lhs.type(), rhs.type());
    }
};

template< class T >
struct value : detail::type_holder<T>
             , detail::value_holder<T>
{
    using value_type
        = typename T::value_type;
    
    value() = delete;

    // from value_type
    constexpr explicit value(const value_type val)
        : value( T(), std::move(val) )
    { }

    constexpr explicit value(const T t, const value_type val)
        : value( t, bit_cast< detail::bits_type<T> >(std::move(val)) )
    { }

    // from field
    constexpr explicit value(const field<T> field)
        : value( field.type(), std::move(field.m_value) )
    { }

    constexpr operator value_type() const
    { return bit_cast<value_type>(this->m_value); }

    // implicit upcast to value<type>
    operator value<type>() const;

    friend std::ostream& operator<<(std::ostream& out, value<T> val)
    {
        return out << T() << " == " << bit_cast<value_type>(val.m_value);
    }

protected:
    // from bits
    constexpr explicit value(const detail::bits_type<T> bits)
        : value(T(), bits())
    { }

    constexpr explicit value(const T t, const detail::bits_type<T> bits)
        : detail::value_holder<T>(t, std::move(bits))
    { }

    friend value<T> operator>> <> (const value<T>, const field<T>);
};

template<>
struct value<type> : detail::type_holder<type>
                   , detail::value_holder<type>
{
    value() = delete;

    // from type and mask_type
    explicit value(const class type t, const value_type val)
        : detail::type_holder<class type>( t )
        , detail::value_holder<class type>( t, std::move(val) )
    { }

    // from field
    template<class T>
    constexpr explicit value(const field<T> f)
        : value( f.type(), std::move(f.m_value) )
    { }
    
    // explicit downcast to value<T>
    template<class T>
    explicit operator value<T>() const
    {
        if (type() != T()) {
            RUNOS_THROW(bad_cast())
                << errinfo_actual_type(type())
                << boost::errinfo_type_info_name( typeid(T).name() )
                << errinfo_requested_type(T());
        }
        using bits_T = detail::bits_type<T>;
        return value<T>(static_cast<bits_T>(m_value));
    }

    friend std::ostream& operator<<(std::ostream& out, value<class type> val)
    {
        return out << val.m_type << " == " << val.m_value;
    }
};

template< class T >
struct mask : detail::type_holder<T>
            , detail::mask_holder<T>
{
    using mask_type
        = typename T::mask_type;
    
    // exact match ctor
    constexpr mask()
        : mask( T(), detail::bits_type<T>().set() )
    { }

    // exact match from constexpr type
    constexpr explicit mask(T)
        : mask()
    { }

    // from mask_type
    constexpr explicit mask(const mask_type val)
        : mask( T(), std::move(val) )
    { }

    constexpr explicit mask(const T t, const mask_type val)
        : mask( t, bit_cast< detail::bits_type<T> >(std::move(val)) )
    { }

    // from field
    constexpr explicit mask(const field<T> field)
        : detail::mask_holder<T>( field.type(), std::move(field.m_mask) )
    { }

    // implicit upcast to value<type>
    operator mask<type>() const;

    friend std::ostream& operator<<(std::ostream& out, mask<T> val)
    {
        return out << T() << " & " << bit_cast<mask_type>(val.m_mask);
    }

protected:
    constexpr explicit mask(const detail::bits_type<T> bits)
        : mask(T(), bits)
    { }

    constexpr explicit mask(const T t, const detail::bits_type<T> bits)
        : detail::mask_holder<T>(t, std::move(bits))
    { }

    friend mask<T> operator& <> (const mask<T>, const mask<T>);
    friend mask<T> operator| <> (const mask<T>, const mask<T>);
    friend mask<T> operator^ <> (const mask<T>, const mask<T>);
    friend mask<T> operator~ <> (const mask<T>);
};

template<>
struct mask<type> : detail::type_holder<type>
                  , detail::mask_holder<type>
{
    // there shouldn't be any public bits ctor!
    // make accessor classes instead
    mask() = delete;

    // exact match ctor
    explicit mask(const class type t)
        : mask( t, bits<>(t.nbits()).set() )
    { }

    // from field
    template<class T>
    explicit mask(const field<T> field)
        : mask( field.type(), std::move(field.m_mask) )
    { }

    // from type and mask_type
    explicit mask(const class type t, const mask_type val)
        : detail::type_holder<class type>( t )
        , detail::mask_holder<class type>( t, std::move(val) )
    { }

    // explicit downcast to mask<T>
    template<class T>
    explicit operator mask<T>() const
    {
        if (type() != T()) {
            RUNOS_THROW(bad_cast()
                << errinfo_actual_type(type())
                << boost::errinfo_type_info_name( typeid(T).name() )
                << errinfo_requested_type(T()));
        }
        using bits_T = detail::bits_type<T>;
        return mask<T>(static_cast<bits_T>(m_mask));
    }

    friend std::ostream& operator<<(std::ostream& out, mask<class type> val)
    {
        return out << val.m_type << " & " << val.m_mask;
    }
};

template< class T >
struct field : detail::type_holder<T>
             , detail::value_holder<T>
             , detail::mask_holder<T>
{
    using value_type
        = typename T::value_type;
    using mask_type
        = typename T::mask_type;

    // wildcard ctor
    constexpr field()
        : field( detail::bits_type<T>(), detail::bits_type<T>() )
    { }

    // exact match ctor
    constexpr explicit field(const value_type val)
        : field( bit_cast< detail::bits_type<T> >(val),
                 detail::bits_type<T>().set() )
    { }

    // fuzzy match ctor
    constexpr explicit field(const value_type val, const mask_type mask)
        : field( bit_cast< detail::bits_type<T> >(std::move(val)),
                 bit_cast< detail::bits_type<T> >(std::move(mask)) )
    { }

    // from value
    constexpr field(const value<T> val)
        : field( val.value_bits(), detail::bits_type<T>().set() )
    { }

    // implicit upcast to field<type>
    operator field<type>() const;

    friend std::ostream& operator<<(std::ostream& out, field<T> f)
    {
        if (f.fuzzy()) {
            return out << "(" << mask<T>(f) << ")" << " == "
                       << bit_cast<value_type>(f.m_value);
        } else {
            return out << value<T>(f);
        }
    }

protected:
    // from bits
    explicit field(const T, const detail::bits_type<T> v,
                            const detail::bits_type<T> m)
        : field(v, m)
    { }

    // main ctor
    explicit field(const detail::bits_type<T> v, const detail::bits_type<T> m)
        : detail::value_holder<T>(T(), std::move(v))
        , detail::mask_holder<T>(T(), std::move(m))
    {
        this->m_value &= this->m_mask;
    }

    friend struct field<type>;
    friend field<T> operator& <> (const value<T>, const mask<T>);
    friend field<T> operator<< <> (const oxm::mask<T>, const typename T::value_type);
    friend field<T> operator== <> (const mask<T>, const typename T::value_type);
};

template<>
struct field<type> : detail::type_holder<type>
                   , detail::value_holder<type>
                   , detail::mask_holder<type>
{
    field() = delete;

    // fuzzy match ctor
    explicit field(class type t, const value_type val, const mask_type mask)
        : detail::type_holder<class type>( t )
        , detail::value_holder<class type>( t, std::move(val) )
        , detail::mask_holder<class type>( t, std::move(mask) )
    {
        m_value &= m_mask;
    }
    
    // wildcard ctor
    explicit field(const class type t)
        : field(t, bits<>(t.nbits()), bits<>(t.nbits()))
    { }

    // exact match ctor
    explicit field(class type t, const value_type val)
        : field(t, std::move(val), bits<>(t.nbits()).set())
    { }

    // from value
    template<class T>
    field(const value<T> val)
        : field( val.type(), val.value_bits() )
    { }

    // explicit downcast to field<T>
    template<class T>
    explicit operator field<T>() const
    {
        if (type() != T()) {
            RUNOS_THROW(bad_cast()
                << errinfo_actual_type(type())
                << boost::errinfo_type_info_name( typeid(T).name() )
                << errinfo_requested_type(T()));
        }
        
        using bits_T = detail::bits_type<T>;
        return field<T>(static_cast<bits_T>(m_value),
                        static_cast<bits_T>(m_mask));
    }

    friend std::ostream& operator<<(std::ostream& out, field<class type> f)
    {
        if (f.fuzzy()) {
            return out << "(" << mask<class type>(f) << ")" << " == "
                       << f.m_value;
        } else {
            return out << value<class type>(f);
        }
    }
};

// implementation
template<class T>
mask<T>::operator mask<type>() const
{ return mask<type>(T(), this->m_mask); }

template<class T>
value<T>::operator value<type>() const
{ return value<type>(T(), this->m_value); }

template<class T>
field<T>::operator field<type>() const
{ return field<type>(T(), this->m_value, this->m_mask); }

// comparasion
template<class T1, class T2>
constexpr bool operator==(mask<T1> const lhs, mask<T2> const rhs) noexcept
{ return (lhs.type() == rhs.type()) && lhs.mask_bits() == rhs.mask_bits(); }

template<class T1, class T2>
constexpr bool operator==(value<T1> const lhs, value<T2> const rhs) noexcept
{ return (lhs.type() == rhs.type()) && lhs.value_bits() == rhs.value_bits(); }

template<class T1, class T2>
constexpr bool operator==(field<T1> const lhs, field<T2> const rhs) noexcept
{ return (lhs.type() == rhs.type()) && lhs.value_bits() == rhs.value_bits()
                                    && lhs.mask_bits()  == rhs.mask_bits(); }

template<class T1, class T2>
constexpr bool operator!=(mask<T1> const lhs, mask<T2> const rhs) noexcept
{ return ! (lhs == rhs); }

template<class T1, class T2>
constexpr bool operator!=(value<T1> const lhs, value<T2> const rhs) noexcept
{ return ! (lhs == rhs); }

template<class T1, class T2>
constexpr bool operator!=(field<T1> const lhs, field<T2> const rhs) noexcept
{ return ! (lhs == rhs); }


// match
template<class T>
constexpr bool operator& (const value<T> lhs, const value<T> rhs)
{ return detail::binop_type(lhs, rhs), lhs == rhs; }

template<class T>
bool operator& (const field<T> lhs, const value<T> rhs)
{ return detail::binop_type(lhs, rhs), lhs == (rhs & mask<T>(lhs)); }

template<class T>
bool operator& (const value<T> lhs, const field<T> rhs)
{ return rhs & lhs; }

template<class T>
bool operator& (const field<T> lhs, const field<T> rhs)
{
    mask<T> m = mask<T>(lhs) & mask<T>(rhs);
    return (value<T>(lhs) & m) == (value<T>(rhs) & m);
}

// rewrite
template<class T>
value<T> operator>> (const field<T> lhs, const value<T> rhs)
{ return binop_type(lhs, rhs), rhs; }

template<class T>
value<T> operator>> (const value<T> lhs, const field<T> rhs)
{ return value<T>(detail::binop_type(lhs, rhs),
                  (lhs.value_bits() & ~rhs.mask_bits()) | rhs.value_bits()); }

template<class T>
value<T> operator>> (const value<T> lhs, const value<T> rhs)
{ return binop_type(lhs, rhs), rhs; }

template<class T>
field<T> operator>> (const field<T> lhs, const field<T> rhs)
{ return (oxm::value<T>(lhs) >> rhs) & (oxm::mask<T>(lhs) | oxm::mask<T>(rhs)); }

// widen
template<class T>
field<T> operator& (const field<T> lhs, const mask<T> rhs)
{ return field<T>{binop_type(lhs, rhs), lhs.value_bits(),
                                        lhs.mask_bits() & rhs.mask_bits()}; }

template<class T>
field<T> operator& (const mask<T> lhs, const field<T> rhs)
{ return rhs & lhs; }

// bit operations
template<class T>
mask<T> operator~ (const mask<T> rhs)
{ return mask<T>(rhs.type(), ~rhs.mask_bits()); }

template<class T>
mask<T> operator| (const mask<T> lhs, const mask<T> rhs)
{ return mask<T>(detail::binop_type(lhs, rhs), lhs.mask_bits() | rhs.mask_bits()); }

template<class T>
mask<T> operator^ (const mask<T> lhs, const mask<T> rhs)
{ return mask<T>(detail::binop_type(lhs, rhs), lhs.mask_bits() ^ rhs.mask_bits()); }

template<class T>
mask<T> operator& (const mask<T> lhs, const mask<T> rhs)
{ return mask<T>(detail::binop_type(lhs, rhs), lhs.mask_bits() & rhs.mask_bits()); }

// construct
template<class T>
field<T> operator& (const value<T> lhs, const mask<T> rhs)
{ return field<T>(binop_type(lhs, rhs), lhs.value_bits(), rhs.mask_bits()); }

template<class T>
field<T> operator& (const mask<T> lhs, const value<T> rhs)
{ return rhs & lhs; }

// construct from type
template<class T, typename>
mask<T> operator& (const T type, const typename T::mask_type mask_val)
{ return mask<T>(type, mask_val); }

template<class T, typename>
mask<T> operator& (const typename T::mask_type mask_val, const T type)
{ return type & mask_val; }

template<class T, typename>
value<T> operator== (const T type, const typename T::value_type val)
{ return value<T>(type, val); }

template<class T, typename>
value<T> operator== (const typename T::value_type val, const T type)
{ return type == val; }

template<class T>
field<T> operator== (const mask<T> m, const typename T::value_type val)
{ return field<T>(bit_cast<detail::bits_type<T>>(val), m.mask_bits()); }

template<class T>
field<T> operator== (const typename T::value_type val, const mask<T> mask)
{ return mask == val; }

template<class T, typename>
value<T> operator<< (const T type, const typename T::value_type val)
{ return value<T>(type, val); }

template<class T, typename>
field<T> operator<< (const oxm::mask<T> m, const typename T::value_type val)
{ return field<T>(bit_cast<detail::bits_type<T>>(val), m.mask_bits()); }

} // namespace oxm
} // namespace runos
