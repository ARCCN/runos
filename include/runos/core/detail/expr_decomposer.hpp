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

#include <runos/core/config.hpp>
#include <string>

#ifndef RUNOS_ENABLE_EXPRESSION_DECOMPOSER
#define RUNOS_DECOMPOSE_EXPR(expr) std::string()
#else

#include <runos/core/demangle.hpp>

#include <fmt/format.h>

#include <boost/hana/core/when.hpp>
#include <boost/hana/core/tag_of.hpp>

#include <functional>
#include <utility> // pair
#include <type_traits>

#include <cstddef> // nullptr_t

#define RUNOS_DECOMPOSE_EXPR(expr) \
    (::runos::expr_decomposer::seed{}->* expr).decompose()

namespace runos {
namespace expr_decomposer {

namespace hana = boost::hana;

template<class Tag, typename = hana::when<true>>
struct dump_value_impl
{
    static void apply( fmt::memory_buffer& out, std::nullptr_t ) { fmt::format_to( std::back_inserter( out ), "{}", "nullptr" ); }
    static void apply( fmt::memory_buffer& out, std::string const & x ) 
      { fmt::format_to( std::back_inserter( out ), "\"{}\"s", x ); }
    static void apply( fmt::memory_buffer& out, const char* x ) 
      { fmt::format_to( std::back_inserter( out ),"\"{}\"", x ); }
    static void apply( fmt::memory_buffer& out, char x ) 
      { fmt::format_to( std::back_inserter( out ), "\'{}\'", x ); }
    static void apply( fmt::memory_buffer& out, bool x ) 
      { fmt::format_to( std::back_inserter( out ), "{}", ( x ? "true" : "false" )); }

    template<class T>
    static void apply( fmt::memory_buffer& out, T const& ) 
      { fmt::format_to( std::back_inserter( out ), "{}{}", demangle(typeid(T).name()), "{...}" ); }
};

struct dump_value_t {
    template<class X>
    void operator()( fmt::memory_buffer& out, X const& x ) const
    {
        using Tag = hana::tag_of_t<X>;
        dump_value_impl<Tag>::apply(out, x);
    }
};
constexpr dump_value_t dump_value{};

template<class X>
struct dump_value_impl<X, hana::when< std::is_arithmetic<X>::value &&
                                    ! std::is_same<X, bool>::value &&
                                    ! std::is_same<X, char>::value >>
{
    static void apply( fmt::memory_buffer& out, X x ) 
      { fmt::format_to_n( std::back_inserter( out ), x, "{}", "{}" ); }
};

template<typename T>
struct is_container
{
    template<typename U>
    static auto test( int ) -> decltype( std::declval<U>().begin() == std::declval<U>().end(), std::true_type() );

    template<typename>
    static auto test( ... ) -> std::false_type;

    static constexpr bool value = std::is_same< decltype( test<T>(0) ), std::true_type >::value;
};

template<class X>
struct dump_value_impl<X, hana::when< is_container<X>::value >>
{
    static void apply( fmt::memory_buffer& out, X const& x )
    {
        fmt::format_to( std::back_inserter( out ), "{" );
        // Dump first element
        auto it = x.begin();
        if (it != x.end()) {
            dump_value(out, *it);
        }
        // Dump other up to 10 elements
        for (int i = 0; i < 10 && it != x.end(); ++i, ++it) {
            fmt::format_to( std::back_inserter( out ), ", " );
            dump_value(out, *it);
        }
        // Dump number of not printed elements in tail
        if (it != x.end()) {
            fmt::format_to( std::back_inserter( out ), ", ... [{} more]", std::distance(it, x.end()));
        }
        fmt::format_to( std::back_inserter( out ), "}" );
    }
};

template<typename T>
struct is_pair
{
    template<typename U>
    static auto test( int ) -> decltype( (std::declval<U>().first, std::declval<U>().second) , std::true_type() );

    template<typename>
    static auto test( ... ) -> std::false_type;

    static constexpr bool value = std::is_same< decltype( test<T>(0) ), std::true_type >::value;
};

template<class X>
struct dump_value_impl<X, hana::when< is_pair<X>::value >>
{
    static void apply( fmt::memory_buffer& out, X const& x )
    {
        fmt::format_to( std::back_inserter( out ), "(" );
        dump_value(out, x.first);
        fmt::format_to( std::back_inserter( out ), ", " );
        dump_value(out, x.second);
        fmt::format_to( std::back_inserter( out ),  ")" );
    }
};

////////////////////////////////////////////////////////////////////////////////

struct bit_shift_left {
    template< class T, class U>
    constexpr decltype(auto) operator()( T&& lhs, U&& rhs ) const
    {
        return std::forward<T>(lhs) << std::forward<U>(rhs);
    }
};

struct bit_shift_right {
    template< class T, class U>
    constexpr decltype(auto) operator()( T&& lhs, U&& rhs ) const
    {
        return std::forward<T>(lhs) >> std::forward<U>(rhs);
    }
};

template<class L>
struct value_expr;

template<class L, class Op, class R>
struct binary_expr;

template<class Op, class L, class R>
binary_expr<L, Op, R>
make_binary_expr( const L& lhs, const char* op, const R& rhs );

template<class L>
value_expr<L>
make_value( L const& lhs );

template<class Final>
struct expr
{
    const Final* self() const { return static_cast<const Final*>(this); }
    Final* self() { return static_cast<Final*>(this); }

    explicit operator bool() const
    {
        return self()->eval();
    }
    
    std::string decompose() const
    try {
        fmt::memory_buffer ret;
        self()->eval(ret);
        return std::string( ret.data());
    } catch (std::bad_alloc&) {
        return std::string();
    }

    // Relational
    template <typename R> auto operator==( R const & rhs ) { return make_binary_expr< std::equal_to<>      >( *self(), " == ", make_value(rhs) ); }
    template <typename R> auto operator!=( R const & rhs ) { return make_binary_expr< std::not_equal_to<>  >( *self(), " != ", make_value(rhs) ); }
    template <typename R> auto operator< ( R const & rhs ) { return make_binary_expr< std::less<>          >( *self(), " < " , make_value(rhs) ); }
    template <typename R> auto operator<=( R const & rhs ) { return make_binary_expr< std::less_equal<>    >( *self(), " <= ", make_value(rhs) ); }
    template <typename R> auto operator> ( R const & rhs ) { return make_binary_expr< std::greater<>       >( *self(), " > " , make_value(rhs) ); }
    template <typename R> auto operator>=( R const & rhs ) { return make_binary_expr< std::greater_equal<> >( *self(), " >= ", make_value(rhs) ); }

    // Arithmetic
    template <typename R> auto operator*( R const & rhs ) { return make_binary_expr< std::multiplies<> >( *self(), " * ", make_value(rhs) ); }
    template <typename R> auto operator/( R const & rhs ) { return make_binary_expr< std::divides<>    >( *self(), " / ", make_value(rhs) ); }
    template <typename R> auto operator%( R const & rhs ) { return make_binary_expr< std::modulus<>    >( *self(), " % ", make_value(rhs) ); }
    template <typename R> auto operator+( R const & rhs ) { return make_binary_expr< std::plus<>       >( *self(), " + ", make_value(rhs) ); }
    template <typename R> auto operator-( R const & rhs ) { return make_binary_expr< std::minus<>      >( *self(), " - ", make_value(rhs) ); }

    // Bitwise
    template <typename R> auto operator&( R const & rhs ) { return make_binary_expr< std::bit_and<> >( *self(), " & ", make_value(rhs) ); }
    template <typename R> auto operator^( R const & rhs ) { return make_binary_expr< std::bit_xor<> >( *self(), " ^ ", make_value(rhs) ); }
    template <typename R> auto operator|( R const & rhs ) { return make_binary_expr< std::bit_or<>  >( *self(), " | ", make_value(rhs) ); }
    template <typename R> auto operator<<( R const & rhs ) { return make_binary_expr< bit_shift_left  >( *self(), " << ", make_value(rhs) ); }
    template <typename R> auto operator>>( R const & rhs ) { return make_binary_expr< bit_shift_right >( *self(), " >> ", make_value(rhs) ); }

    // Logical
    // This overloads change semantics of the expression, but if we require purity it's ok.
    template <typename R> auto operator&&( R const & rhs ) { return make_binary_expr< std::logical_and<> >( *self(), " && ", make_value(rhs) ); }
    template <typename R> auto operator||( R const & rhs ) { return make_binary_expr< std::logical_or<>  >( *self(), " || ", make_value(rhs) ); }
};

template<class L>
struct value_expr : expr< value_expr<L> >
{
    const L& lhs;

    value_expr( const L& lhs ) : lhs(lhs) {}

    decltype(auto) eval() const
    {
        return lhs;
    }

    decltype(auto) eval( fmt::memory_buffer& out ) const
    {
        dump_value(out, lhs);
        return lhs;
    }

};

template<class L>
value_expr<L>
make_value( L const& lhs )
{
    return { lhs };
}

template<class L, class Op, class R>
struct binary_expr : expr< binary_expr<L, Op, R> >
{
    L lhs;
    const char* const op;
    R rhs;

    binary_expr( L lhs, const char* op, R rhs ) : lhs(lhs), op(op), rhs(rhs) { }

    auto eval() const
    {
        return Op()(lhs.eval(), rhs.eval());
    }

    auto eval( fmt::memory_buffer& out ) const
    {
        auto l = lhs.eval(out);
        fmt::format_to( std::back_inserter( out ), "{}", op );
        auto r = rhs.eval(out);
        return Op()(std::move(l), std::move(r));
    }
};

template<class Op, class L, class R>
binary_expr<L, Op, R>
make_binary_expr( const L& lhs, const char* op, const R& rhs )
{
    return { lhs, op, rhs };
}

struct seed
{
    template<class T>
    value_expr<T> operator->*( T const& v )
    {
        return value_expr<T>( v );
    }
};

} // expr_decomposer
} // runos

#endif
