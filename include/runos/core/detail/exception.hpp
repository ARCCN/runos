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

#include <runos/core/exception.hpp>

#include <type_traits>
#include <stdexcept>
#include <utility>

namespace runos {
namespace detail {

struct std_exception_shim : std::exception
{
    explicit std_exception_shim(const char* msg) noexcept
        : cmsg(msg)
    { }

    explicit std_exception_shim(std::string msg) noexcept
        : smsg(std::move(msg))
    { }

    const char* what() const noexcept override
    {
        return cmsg ? cmsg : smsg.c_str();
    }

    const char* cmsg {nullptr};
    std::string smsg;
};

/**
 * Choose appropriate std::exception class to derive from
 * and validate user exception type.
 */
template<class Exception>
class std_exception_base {
    static constexpr bool runtime_error
        = std::is_base_of<runtime_error_tag, Exception>::value;

    static constexpr bool logic_error
        = std::is_base_of<logic_error_tag, Exception>::value;

    static constexpr bool invalid_argument
        = std::is_base_of<invalid_argument_tag, Exception>::value;

    static_assert(not(runtime_error && logic_error),
        "Deriving from both runtime_error_tag and logic_error_tag is senseless.");

    static_assert(std::is_nothrow_move_constructible<Exception>::value,
        "Exception type must be nothrow move constructible.");

    static_assert(not std::is_base_of<std::exception, Exception>::value,
        "Exception type must not derive from std::exception. Use tags instead.");

    static_assert(std::is_base_of<exception_root, Exception>::value,
        "Exception must derive from exception_root");

public:
    using type =
        std::conditional_t<runtime_error, std::runtime_error,
        std::conditional_t<invalid_argument, std::invalid_argument,
        std::conditional_t<logic_error, std::logic_error,
                           std_exception_shim>>>;
};

template<class Exception>
using std_exception_base_t = typename std_exception_base<Exception>::type;

template<class Exception, class StdException = std_exception_base_t<Exception>>
struct adapted_exception : StdException
                         , extended_exception
                         , Exception
{
    explicit adapted_exception(extended_exception&& ex,
                               Exception&& e,
                               std::string&& msg) noexcept
        : StdException{std::move(msg)}
        , extended_exception{std::move(ex)}
        , Exception{std::move(e)}
    { }

    explicit adapted_exception(extended_exception&& ex,
                               Exception&& e,
                               const char* fmt) noexcept
        : StdException{fmt}
        , extended_exception{std::move(ex)}
        , Exception{std::move(e)}
    { }
};

template<class Exception>
auto adapt_exception(extended_exception&& ex, Exception&& e, const char* fmt) noexcept
{
    using E = std::remove_reference_t<Exception>;
    return adapted_exception<E>{std::move(ex), std::forward<Exception>(e), fmt};
}

template<class Exception>
auto adapt_exception(extended_exception&& ex, Exception&& e, std::string&& msg) noexcept
{
    using E = std::remove_reference_t<Exception>;
    return adapted_exception<E>{std::move(ex), std::forward<Exception>(e), std::move(msg)};
}

} // detail
} // runos
