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
#include <runos/core/detail/assert.hpp>
#include <runos/core/source_location.hpp>
#include <runos/core/detail/expr_decomposer.hpp>

#include <boost/config.hpp> // BOOST_LIKELY

#define RUNOS_CHECK(expr, ...) \
    (BOOST_LIKELY(!!(expr)) ? ((void)0) :                             \
        ::runos::detail::assertion_failed(#expr,                      \
                                          RUNOS_DECOMPOSE_EXPR(expr), \
                                          CURRENT_SOURCE_LOCATION,    \
                                          ##__VA_ARGS__))

#if defined(RUNOS_ENABLE_ASSERTIONS) || !defined(NDEBUG)
#define RUNOS_ASSERT(...) RUNOS_CHECK(__VA_ARGS__)
#else
#define RUNOS_ASSERT(...) ((void)0)
#endif

#ifdef RUNOS_USE_SHORT_MACRO
#define CHECK RUNOS_CHECK
#define ASSERT RUNOS_ASSERT
#endif
