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
 
#include <runos/core/source_location.hpp>
#include <runos/core/nothrow_format.hpp>

#include <string>

namespace runos {
namespace detail {

[[ noreturn ]] void assertion_failed(const char *       expr,
                                     std::string        expr_decomposed,
                                     source_location    where,
                                     std::string        msg);

[[ noreturn ]] void assertion_failed(const char *       expr,
                                     std::string        expr_decomposed,
                                     source_location    where,
                                     const char *       msg);

[[ noreturn ]] inline
void assertion_failed(const char *       expr,
                      std::string        expr_decomposed,
                      source_location    where)
{
    assertion_failed(expr, expr_decomposed, where, "");
}

template<class Arg, class ... Args>
[[ noreturn ]] void assertion_failed(const char *       expr,
                                     std::string        expr_decomposed,
                                     source_location    where,
                                     const char *       format_str,
                                     Arg  const&        arg1,
                                     Args const& ...    args)
{
    std::string msg = nothrow_format(format_str, arg1, args...);
    if (msg.empty()) {
        assertion_failed(expr, std::move(expr_decomposed), where, format_str);
    } else {
        assertion_failed(expr, std::move(expr_decomposed), where,
                         static_cast<std::string&&>(msg));
    }
}

} // detail
} // runos
