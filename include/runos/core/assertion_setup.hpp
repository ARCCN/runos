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

#include <runos/core/assert.hpp>
#include <runos/core/exception.hpp>
#include <functional>

namespace runos {

/**
 * Throws then assetion was failed.
 * Must be catched only by outermost (fault tolerance) handler.
 */
struct assertion_error : exception_root, logic_error_tag
{ };

/**
 * Assertion handler executed at point of failure.
 */
using assertion_failure_hook
    = std::function<void(std::string const&,    // msg
                         const char*,           // expr
                         std::string const&,    // expr_decomposed
                         source_location const& // where
                        )>;

/*
 * Adds a new assertion failure handler to the list.
 * This handlers executed at the point of assertion failure in unspecified order.
 * Must be called after `main()`.
 * Thread-safe.
 */
void on_assertion_failed(assertion_failure_hook hook);

} // runos
