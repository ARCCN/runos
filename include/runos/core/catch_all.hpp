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
#include <runos/core/safe_ptr.hpp>
#include <runos/core/detail/precatch_shim.hpp>

#include <boost/exception/exception.hpp>

#include <string>
#include <exception>
#include <ostream>
#include <utility>
#include <optional>
#include <type_traits>

namespace runos {

struct unknown_exception_tag { };

struct diagnostic_information
{
    diagnostic_information() noexcept;
    explicit diagnostic_information( unknown_exception_tag );
    explicit diagnostic_information( std::exception_ptr e );
    explicit diagnostic_information( extended_exception const& e );
    explicit diagnostic_information( std::exception const& e );
    explicit diagnostic_information( boost::exception const& e );

    /**
     * Get diagnostic information from current exception.
     */
    static diagnostic_information get() noexcept;

    /**
     * Print formatted message to logger.
     */
    void log(/*logger*/) const;

    /**
     * Error message.
     */
    std::string what;

    /**
     * Location in the source code.
     */
    std::string where;

    /**
     * Type of exception.
     */
    std::string exception_type;

    /**
     * Condition leads to the error.
     */
    std::string condition;

    /**
     * Error-specific data.
     */
    std::forward_list<std::string> with;

    /**
     * Information for nested exception.
     */
    safe::unique_ptr<diagnostic_information> nested;
};

std::ostream& operator<<(std::ostream& out, diagnostic_information const& diag);

template<class F>
std::optional<diagnostic_information> catch_all(F&& f, bool backtrace = true) noexcept
try {
    static_assert( std::is_same< std::result_of_t<F()>, void >::value,
        "catch_all functor must be callable without arguments and return void" );

    try {
        //if (backtrace) {
        //    precatch_shim([&]() {
        //        f();
        //    });
        //} else {
            f();
        //}
        return std::nullopt;
    } catch( extended_exception const& e ) {
        return diagnostic_information{e};
    } catch( boost::exception const& e ) {
        return diagnostic_information{e};
    } catch( std::exception const& e ) {
        return diagnostic_information{e};
    } catch( ... ) {
        return diagnostic_information{unknown_exception_tag{}};
    }
} catch (std::bad_alloc&) {
    return diagnostic_information{};
}

template<class F>
std::optional<diagnostic_information> catch_all_and_log(F&& f/*, logger_t logger*/) noexcept
{
    auto diag = catch_all(std::forward<F>(f), true);
    if (diag) {
        diag->log(/*logger*/);
    }
    return diag;
}

} // runos
