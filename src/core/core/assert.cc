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

#include <runos/core/assertion_setup.hpp>
#include <runos/core/throw.hpp>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_factories.hpp>

#include <utility>
#include <forward_list>

namespace runos {

static boost::shared_mutex assertion_hooks_mutex;
static std::forward_list<assertion_failure_hook> assertion_hooks;

void on_assertion_failed(assertion_failure_hook hook)
{
    auto lock = boost::make_unique_lock(assertion_hooks_mutex);
    assertion_hooks.push_front(std::move(hook));
}

namespace detail {

void run_assertion_hooks(std::string const&     msg,
                         const char *           expr,
                         std::string const&     expr_decomposed,
                         source_location const& where) noexcept
{
    try {
        boost::shared_lock<boost::shared_mutex> lock(assertion_hooks_mutex);
        for (auto& hook : assertion_hooks) try {
            hook(msg, expr, expr_decomposed, where);
        } catch (...) { }
    } catch (...) { }
}

[[ noreturn ]] void assertion_failed(const char *       expr,
                                     std::string        expr_decomposed,
                                     source_location    where,
                                     const char *       msg)
{
    run_assertion_hooks(msg, expr, expr_decomposed, where);
    throw adapt_exception(
        extended_exception{
            std::move(where),
            typeid(assertion_error),
            expr,
            std::move(expr_decomposed)
        },
        assertion_error(),
        msg
    );
}

[[ noreturn ]] void assertion_failed(const char *       expr,
                                     std::string        expr_decomposed,
                                     source_location    where,
                                     std::string        msg)
{
    run_assertion_hooks(msg, expr, expr_decomposed, where);
    throw adapt_exception(
        extended_exception{
            std::move(where),
            typeid(assertion_error),
            expr,
            std::move(expr_decomposed)
        },
        assertion_error(),
        std::move(msg)
    );
}

} // detail

} // runos
