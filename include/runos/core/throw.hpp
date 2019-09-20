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
#include <runos/core/detail/exception.hpp>
#include <runos/core/detail/expr_decomposer.hpp>
#include <runos/core/nothrow_format.hpp>

#include <boost/config.hpp> // BOOST_UNLIKELY

namespace runos {

/**
 * Merges user exception hierarchy with `std::exception`.
 *         -------------   ----------------------   ------------------
 *         | Exception |   | extended_exception |   | std::exception |
 *         -------------   ----------------------   ------------------
 *               ^                    ^                     ^
 *               |                    |                     |
 *               |             --------------               |
 *               ------------- | ResultType | ---------------
 *                             --------------
 */

template<class Exception, class Arg, class ... Args>
auto make_exception(extended_exception&& ex, Exception&& e,
                    const char* format_str, Arg const& arg1, Args const& ... args)
/* This methods should be noexcept but in strange conditions (compiler bug?)
 * this causes Clang to produce segfaulty code for THROW_IF.
 */
{
    std::string what = nothrow_format(format_str, arg1, args...);

    return what.empty() ? detail::adapt_exception(std::move(ex),
                                                  std::forward<Exception>(e),
                                                  format_str)
                        : detail::adapt_exception(std::move(ex),
                                                  std::forward<Exception>(e),
                                                  std::move(what));
}

template<class Exception>
auto make_exception(extended_exception&& ex, Exception&& e)
{
    return detail::adapt_exception(std::move(ex),
                                   std::forward<Exception>(e),
                                   "");
}

template<class Exception>
auto make_exception(extended_exception&& ex, Exception&& e, const char* msg)
{
    return detail::adapt_exception(std::move(ex),
                                   std::forward<Exception>(e),
                                   msg);
}

template<class Exception>
auto make_exception(extended_exception&& ex, Exception&& e, std::string msg)
{
    return detail::adapt_exception(std::move(ex),
                                   std::forward<Exception>(e),
                                   std::move(msg));
}

#define RUNOS_EXTENDED_EXCEPTION_2(ex, ...) \
    extended_exception{CURRENT_SOURCE_LOCATION, typeid((ex))}

#define RUNOS_EXTENDED_EXCEPTION_4(expr, ex, ...) \
    extended_exception{CURRENT_SOURCE_LOCATION, typeid((ex)), #expr, RUNOS_DECOMPOSE_EXPR(expr)}

#define RUNOS_MAKE_EXCEPTION(...) \
    (::runos::make_exception(RUNOS_EXTENDED_EXCEPTION_2(__VA_ARGS__), __VA_ARGS__))

#define RUNOS_THROW(...) throw RUNOS_MAKE_EXCEPTION(__VA_ARGS__)
#define RUNOS_THROW_WITH_NESTED(...) std::throw_with_nested(RUNOS_MAKE_EXCEPTION(__VA_ARGS__))
#define RUNOS_MAKE_EXCEPTION_PTR(...) std::make_exception_ptr(RUNOS_MAKE_EXCEPTION(__VA_ARGS__))

#define RUNOS_THROW_IF(expr, ...) (BOOST_UNLIKELY(!!(expr)) ?                           \
    (throw ::runos::make_exception(RUNOS_EXTENDED_EXCEPTION_4(expr, __VA_ARGS__),\
                                  __VA_ARGS__)) : (void)0)

#ifdef RUNOS_USE_SHORT_MACRO
#define THROW              RUNOS_THROW
#define THROW_IF           RUNOS_THROW_IF
#define THROW_WITH_NESTED  RUNOS_THROW_WITH_NESTED
#define MAKE_EXCEPTION     RUNOS_MAKE_EXCEPTION
#define MAKE_EXCEPTION_PTR RUNOS_MAKE_EXCEPTION_PTR
#endif

} // runos
