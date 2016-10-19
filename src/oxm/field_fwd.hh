#pragma once

#include <utility>
#include "type_fwd.hh"

namespace runos {
namespace oxm {

// fwd
template<class T = type>
struct field;
template<class T = type>
struct value;
template<class T = type>
struct mask;

template<class T>
value<T> operator>> (const value<T> lhs, const field<T> rhs);

// comparasion
template<class T1, class T2>
constexpr bool operator==(mask<T1> const lhs, mask<T2> const rhs) noexcept;

template<class T1, class T2>
constexpr bool operator==(value<T1> const lhs, value<T2> const rhs) noexcept;

template<class T1, class T2>
constexpr bool operator==(field<T1> const lhs, field<T2> const rhs) noexcept;

template<class T1, class T2>
constexpr bool operator!=(mask<T1> const lhs, mask<T2> const rhs) noexcept;

template<class T1, class T2>
constexpr bool operator!=(value<T1> const lhs, value<T2> const rhs) noexcept;

template<class T1, class T2>
constexpr bool operator!=(field<T1> const lhs, field<T2> const rhs) noexcept;

// match
template<class T>
constexpr bool operator& (const value<T> lhs, const value<T> rhs);

template<class T>
bool operator& (const field<T> lhs, const value<T> rhs);

template<class T>
bool operator& (const value<T> lhs, const field<T> rhs);

template<class T>
bool operator& (const field<T> lhs, const field<T> rhs);

// rewrite
template<class T>
value<T> operator>> (const field<T> lhs, const value<T> rhs);

template<class T>
value<T> operator>> (const value<T> lhs, const field<T> rhs);

template<class T>
value<T> operator>> (const value<T> lhs, const value<T> rhs);

template<class T>
field<T> operator>> (const field<T> lhs, const field<T> rhs);

// widen
template<class T>
field<T> operator& (const field<T> lhs, const mask<T> rhs);

template<class T>
field<T> operator& (const mask<T> lhs, const field<T> rhs);

// bit operations
template<class T>
mask<T> operator~ (const mask<T> rhs);

template<class T>
mask<T> operator| (const mask<T> lhs, const mask<T> rhs);

template<class T>
mask<T> operator^ (const mask<T> lhs, const mask<T> rhs);

template<class T>
mask<T> operator& (const mask<T> lhs, const mask<T> rhs);

// construct
template<class T>
field<T> operator& (const value<T> lhs, const mask<T> rhs);

template<class T>
field<T> operator& (const mask<T> lhs, const value<T> rhs);

// construct from type
template<class T, typename = typename std::enable_if<std::is_base_of<type, T>::value>::type >
mask<T> operator& (const T type, const typename T::mask_type mask_val);

template<class T, typename = typename std::enable_if<std::is_base_of<type, T>::value>::type >
mask<T> operator& (const typename T::mask_type mask_val, const T type);

template<class T, typename = typename std::enable_if<std::is_base_of<type, T>::value>::type >
value<T> operator== (const T type, const typename T::value_type val);

template<class T, typename = typename std::enable_if<std::is_base_of<type, T>::value>::type >
value<T> operator== (const typename T::value_type val, const T type);

template<class T>
field<T> operator== (const mask<T> mask, const typename T::value_type val);

template<class T>
field<T> operator== (const typename T::value_type val, const mask<T> mask);

template<class T, typename = typename std::enable_if<std::is_base_of<type, T>::value>::type >
value<T> operator<< (const T type, const typename T::value_type val);

template<class T, typename = typename std::enable_if<std::is_base_of<type, T>::value>::type >
field<T> operator<< (const oxm::mask<T> m, const typename T::value_type val);

}
}
