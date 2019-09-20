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

#if defined(RUNOS_HAVE_STD_SOURCE_LOCATION)

#include <source_location>

namespace runos { using std::source_location; }
#define CURRENT_SOURCE_LOCATION std::source_location::current()

#elif defined(RUNOS_HAVE_STD_EXPERIMENTAL_SOURCE_LOCATION) && \
      defined(RUNOS_USE_STD_EXPERIMENTAL_SOURCE_LOCATION)

#include <experimental/source_location>

namespace runos { using std::experimental::source_location; }
#define CURRENT_SOURCE_LOCATION std::experimental::source_location::current()

#else

#include <cstdint>

namespace runos {

struct source_location
{
    constexpr auto function_name() const { return function_name_; }
    constexpr auto file_name() const { return file_name_; }
    constexpr auto line() const { return line_; }
    constexpr auto column() const { return column_; }

    const char* function_name_;
    const char* file_name_;
    std::uint_least32_t line_;
    std::uint_least32_t column_;
};

} // runos

#define CURRENT_SOURCE_LOCATION  \
    ::runos::source_location {   \
        __PRETTY_FUNCTION__,     \
        __FILE__,                \
        __LINE__,                \
        0                        \
    }

#endif
